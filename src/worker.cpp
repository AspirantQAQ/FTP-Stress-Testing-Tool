#include "worker.h"
#include <curl/curl.h>
#include <cstring>
#include <thread>
#include <chrono>

struct UploadContext {
    const uint8_t* data;
    uint64_t size;
    uint64_t offset;
};

static size_t upload_read_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    auto* ctx = static_cast<UploadContext*>(userdata);
    uint64_t remaining = ctx->size - ctx->offset;
    uint64_t to_copy = std::min(static_cast<uint64_t>(size * nitems), remaining);
    if (to_copy == 0) return 0;
    std::memcpy(buffer, ctx->data + ctx->offset, to_copy);
    ctx->offset += to_copy;
    return to_copy;
}

static size_t download_discard_callback(char*, size_t size, size_t nmemb, void*) {
    return size * nmemb;
}

Worker::Worker(int id, const Config& cfg, const std::vector<uint8_t>& data,
               StatsCollector& stats, std::atomic<bool>& stop_flag)
    : id_(id), cfg_(cfg), data_(data), stats_(stats), stop_flag_(stop_flag) {
    remote_filename_ = "stress_test_" + std::to_string(id_);
}

void Worker::start() {
    thread_ = std::make_unique<std::thread>(&Worker::run, this);
}

void Worker::join() {
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

void Worker::run() {
    while (!stop_flag_.load()) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        bool upload_ok = upload_file(curl);
        if (upload_ok) {
            download_file(curl);
        }

        if (!upload_ok) {
            long response_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            delete_remote_file(curl);
            curl_easy_cleanup(curl);
            if (response_code == 530) {
                stop_flag_.store(true);
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } else {
            delete_remote_file(curl);
            curl_easy_cleanup(curl);
        }
    }
}

bool Worker::upload_file(void* curl) {
    CURL* c = static_cast<CURL*>(curl);
    std::string url = "ftp://" + cfg_.host + ":" + std::to_string(cfg_.port) + "/" + remote_filename_;

    UploadContext ctx{data_.data(), data_.size(), 0};

    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_USERNAME, cfg_.username.c_str());
    curl_easy_setopt(c, CURLOPT_PASSWORD, cfg_.password.c_str());
    curl_easy_setopt(c, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(c, CURLOPT_READFUNCTION, upload_read_callback);
    curl_easy_setopt(c, CURLOPT_READDATA, &ctx);
    curl_easy_setopt(c, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(data_.size()));
    curl_easy_setopt(c, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, static_cast<long>(cfg_.timeout));
    curl_easy_setopt(c, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_DEFAULT);

    CURLcode res = curl_easy_perform(c);
    if (res == CURLE_OK) {
        stats_.upload_success++;
        stats_.upload_bytes += data_.size();
        return true;
    } else {
        stats_.upload_fail++;
        return false;
    }
}

bool Worker::download_file(void* curl) {
    CURL* c = static_cast<CURL*>(curl);
    std::string url = "ftp://" + cfg_.host + ":" + std::to_string(cfg_.port) + "/" + remote_filename_;

    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_USERNAME, cfg_.username.c_str());
    curl_easy_setopt(c, CURLOPT_PASSWORD, cfg_.password.c_str());
    curl_easy_setopt(c, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, download_discard_callback);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, static_cast<long>(cfg_.timeout));
    curl_easy_setopt(c, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_DEFAULT);

    CURLcode res = curl_easy_perform(c);
    if (res == CURLE_OK) {
        double downloaded = 0;
        curl_easy_getinfo(c, CURLINFO_SIZE_DOWNLOAD, &downloaded);
        stats_.download_success++;
        stats_.download_bytes += static_cast<uint64_t>(downloaded);
        return true;
    } else {
        stats_.download_fail++;
        return false;
    }
}

bool Worker::delete_remote_file(void* curl) {
    CURL* c = static_cast<CURL*>(curl);
    std::string url = "ftp://" + cfg_.host + ":" + std::to_string(cfg_.port) + "/";
    struct curl_slist* cmds = nullptr;
    cmds = curl_slist_append(cmds, ("DELE " + remote_filename_).c_str());

    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_USERNAME, cfg_.username.c_str());
    curl_easy_setopt(c, CURLOPT_PASSWORD, cfg_.password.c_str());
    curl_easy_setopt(c, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(c, CURLOPT_QUOTE, cmds);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, download_discard_callback);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(c);
    curl_slist_free_all(cmds);

    return res == CURLE_OK;
}
