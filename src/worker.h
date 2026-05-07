#pragma once
#include "config.h"
#include "stats_collector.h"
#include <atomic>
#include <vector>
#include <thread>
#include <memory>

class Worker {
public:
    Worker(int id, const Config& cfg, const std::vector<uint8_t>& data,
           StatsCollector& stats, std::atomic<bool>& stop_flag);
    void start();
    void join();

private:
    void run();
    bool upload_file(void* curl);
    bool download_file(void* curl);
    bool delete_remote_file(void* curl);

    int id_;
    const Config& cfg_;
    const std::vector<uint8_t>& data_;
    StatsCollector& stats_;
    std::atomic<bool>& stop_flag_;
    std::unique_ptr<std::thread> thread_;
    std::string remote_filename_;
};
