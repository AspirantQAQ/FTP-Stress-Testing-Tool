#pragma once
#include <atomic>
#include <cstdint>
#include <vector>

struct Snapshot {
    int elapsed_seconds = 0;
    uint64_t interval_upload_bytes = 0;
    uint64_t interval_download_bytes = 0;
    uint64_t upload_bytes = 0;
    uint64_t download_bytes = 0;
    uint64_t upload_success = 0;
    uint64_t upload_fail = 0;
    uint64_t download_success = 0;
    uint64_t download_fail = 0;
};

class StatsCollector {
public:
    std::atomic<uint64_t> upload_success{0};
    std::atomic<uint64_t> upload_fail{0};
    std::atomic<uint64_t> download_success{0};
    std::atomic<uint64_t> download_fail{0};
    std::atomic<uint64_t> upload_bytes{0};
    std::atomic<uint64_t> download_bytes{0};

    Snapshot take_snapshot(int elapsed_seconds);

private:
    uint64_t last_upload_bytes_ = 0;
    uint64_t last_download_bytes_ = 0;
};
