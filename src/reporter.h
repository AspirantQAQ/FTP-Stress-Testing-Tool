#pragma once
#include "stats_collector.h"
#include "config.h"
#include <vector>
#include <string>

class Reporter {
public:
    Reporter(const Config& cfg, const StatsCollector& stats, int thread_count);

    void print_realtime(const Snapshot& snap);
    void generate_html_report(const std::vector<Snapshot>& snapshots,
                              double total_seconds,
                              const std::string& output_path);

private:
    std::string format_speed(uint64_t bytes, int seconds);

    const Config& cfg_;
    const StatsCollector& stats_;
    int thread_count_;
};
