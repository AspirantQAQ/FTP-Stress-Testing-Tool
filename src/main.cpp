#include "config.h"
#include "worker.h"
#include "stats_collector.h"
#include "reporter.h"
#include "utils.h"
#include <curl/curl.h>
#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <atomic>
#include <csignal>
#include <iomanip>

static std::atomic<bool> g_stop_flag{false};

static void signal_handler(int) {
    g_stop_flag.store(true);
}

#ifdef _WIN32
#include <windows.h>
static BOOL WINAPI win_ctrl_handler(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT) {
        g_stop_flag.store(true);
        return TRUE;
    }
    return FALSE;
}
#endif

static void setup_signal_handler() {
#ifndef _WIN32
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
#else
    SetConsoleCtrlHandler(win_ctrl_handler, TRUE);
#endif
}

int main(int argc, char* argv[]) {
    Config cfg = parse_args(argc, argv);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    setup_signal_handler();

    auto test_data = generate_random_data(cfg.file_size);
    StatsCollector stats;
    Reporter reporter(cfg, stats, cfg.threads);

    std::cout << "FTP Stress Test Starting..." << std::endl;
    std::cout << "  Server: " << cfg.host << ":" << cfg.port << std::endl;
    std::cout << "  Threads: " << cfg.threads << std::endl;
    std::cout << "  Duration: " << cfg.duration << "s" << std::endl;
    std::cout << "  File size: " << format_bytes(cfg.file_size) << std::endl;
    std::cout << std::endl;

    std::vector<std::unique_ptr<Worker>> workers;
    for (int i = 0; i < cfg.threads; ++i) {
        workers.push_back(std::make_unique<Worker>(i, cfg, test_data, stats, g_stop_flag));
    }
    for (auto& w : workers) w->start();

    std::vector<Snapshot> snapshots;
    auto start_time = std::chrono::steady_clock::now();

    while (!g_stop_flag.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(cfg.interval));
        auto now = std::chrono::steady_clock::now();
        int elapsed = static_cast<int>(
            std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count());

        if (elapsed >= cfg.duration) {
            g_stop_flag.store(true);
        }

        auto snap = stats.take_snapshot(elapsed);
        snapshots.push_back(snap);
        reporter.print_realtime(snap);
    }

    for (auto& w : workers) w->join();

    auto end_time = std::chrono::steady_clock::now();
    double total_seconds = std::chrono::duration<double>(end_time - start_time).count();

    std::string report_path = cfg.report_path;
    if (report_path.empty()) {
        auto now_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::ostringstream oss;
        oss << "report_" << std::put_time(std::localtime(&now_t), "%Y%m%d_%H%M%S") << ".html";
        report_path = oss.str();
    }

    reporter.generate_html_report(snapshots, total_seconds, report_path);

    std::cout << std::endl;
    std::cout << "Test completed in " << static_cast<int>(total_seconds) << " seconds." << std::endl;
    std::cout << "Report saved to: " << report_path << std::endl;

    curl_global_cleanup();
    return 0;
}
