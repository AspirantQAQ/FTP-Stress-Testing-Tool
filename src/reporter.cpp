#include "reporter.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

Reporter::Reporter(const Config& cfg, const StatsCollector& stats, int thread_count)
    : cfg_(cfg), stats_(stats), thread_count_(thread_count) {}

std::string Reporter::format_speed(uint64_t bytes, int seconds) {
    if (seconds <= 0) return "0 B/s";
    double speed = static_cast<double>(bytes) / seconds;
    const char* units[] = {"B/s", "KB/s", "MB/s", "GB/s"};
    int unit_idx = 0;
    while (speed >= 1024.0 && unit_idx < 3) {
        speed /= 1024.0;
        unit_idx++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << speed << " " << units[unit_idx];
    return oss.str();
}

void Reporter::print_realtime(const Snapshot& snap) {
    uint64_t interval_total = snap.interval_upload_bytes + snap.interval_download_bytes;
    std::string speed = format_speed(interval_total, cfg_.interval);

    std::cout << "[" << format_elapsed(snap.elapsed_seconds) << "] "
              << "Threads: " << thread_count_ << " | "
              << "Upload: " << std::setw(5) << snap.upload_success << " success, "
              << std::setw(3) << snap.upload_fail << " fail | "
              << "Download: " << std::setw(5) << snap.download_success << " success, "
              << std::setw(3) << snap.download_fail << " fail | "
              << "Speed: " << speed
              << std::endl;
}

void Reporter::generate_html_report(const std::vector<Snapshot>& snapshots,
                                     double total_seconds,
                                     const std::string& output_path) {
    std::ofstream file(output_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open report file: " << output_path << std::endl;
        return;
    }

    std::ostringstream labels, upload_speed, download_speed, total_speed;
    std::ostringstream upload_success_data, download_success_data, fail_data;

    labels << "[";
    upload_speed << "[";
    download_speed << "[";
    total_speed << "[";
    upload_success_data << "[";
    download_success_data << "[";
    fail_data << "[";

    for (size_t i = 0; i < snapshots.size(); ++i) {
        const auto& s = snapshots[i];
        if (i > 0) {
            labels << ","; upload_speed << ","; download_speed << ",";
            total_speed << ","; upload_success_data << ",";
            download_success_data << ","; fail_data << ",";
        }
        labels << "\"" << format_elapsed(s.elapsed_seconds) << "\"";

        double interval = (i == 0) ? cfg_.interval :
            (snapshots[i].elapsed_seconds - snapshots[i-1].elapsed_seconds);
        if (interval <= 0) interval = cfg_.interval;

        double up_mb = static_cast<double>(s.interval_upload_bytes) / interval / (1024.0 * 1024.0);
        double down_mb = static_cast<double>(s.interval_download_bytes) / interval / (1024.0 * 1024.0);

        upload_speed << std::fixed << std::setprecision(2) << up_mb;
        download_speed << std::fixed << std::setprecision(2) << down_mb;
        total_speed << std::fixed << std::setprecision(2) << (up_mb + down_mb);

        upload_success_data << s.upload_success;
        download_success_data << s.download_success;
        fail_data << (s.upload_fail + s.download_fail);
    }

    labels << "]";
    upload_speed << "]";
    download_speed << "]";
    total_speed << "]";
    upload_success_data << "]";
    download_success_data << "]";
    fail_data << "]";

    const auto& last = snapshots.empty() ? Snapshot{} : snapshots.back();
    uint64_t total_bytes = last.upload_bytes + last.download_bytes;
    std::string avg_speed = format_speed(total_bytes, static_cast<int>(total_seconds));

    file << R"(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<title>FTP 压力测试报告</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
<style>
body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; max-width: 960px; margin: 0 auto; padding: 20px; background: #f5f5f5; }
h1 { color: #333; }
.info { background: #fff; padding: 16px; border-radius: 8px; margin-bottom: 20px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
.cards { display: grid; grid-template-columns: repeat(4, 1fr); gap: 12px; margin-bottom: 20px; }
.card { background: #fff; padding: 16px; border-radius: 8px; text-align: center; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
.card .value { font-size: 24px; font-weight: bold; color: #1976d2; }
.card .label { font-size: 13px; color: #666; margin-top: 4px; }
.chart-container { background: #fff; padding: 16px; border-radius: 8px; margin-bottom: 20px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
</style>
</head>
<body>
<h1>FTP 压力测试报告</h1>
<div class="info">
<p>服务器: )" << cfg_.host << ":" << cfg_.port << R"( | 用户: )" << cfg_.username
    << R"( | 线程数: )" << cfg_.threads
    << R"( | 时长: )" << cfg_.duration << "s"
    << R"( | 文件大小: )" << format_bytes(cfg_.file_size)
    << R"(</p>
</div>
<div class="cards">
<div class="card"><div class="value">)" << last.upload_success << R"(</div><div class="label">上传成功</div></div>
<div class="card"><div class="value">)" << last.download_success << R"(</div><div class="label">下载成功</div></div>
<div class="card"><div class="value">)" << (last.upload_fail + last.download_fail) << R"(</div><div class="label">总失败</div></div>
<div class="card"><div class="value">)" << avg_speed << R"(</div><div class="label">平均速率</div></div>
</div>
<div class="chart-container"><canvas id="speedChart"></canvas></div>
<div class="chart-container"><canvas id="countChart"></canvas></div>
<script>
new Chart(document.getElementById('speedChart'), {
    type: 'line',
    data: {
        labels: )" << labels.str() << R"(,
        datasets: [
            { label: '上传速率 (MB/s)', data: )" << upload_speed.str() << R"(, borderColor: '#1976d2', tension: 0.3 },
            { label: '下载速率 (MB/s)', data: )" << download_speed.str() << R"(, borderColor: '#388e3c', tension: 0.3 },
            { label: '合计速率 (MB/s)', data: )" << total_speed.str() << R"(, borderColor: '#d32f2f', tension: 0.3 }
        ]
    },
    options: { responsive: true, plugins: { title: { display: true, text: '传输速率' } }, scales: { y: { title: { display: true, text: 'MB/s' } } } }
});
new Chart(document.getElementById('countChart'), {
    type: 'line',
    data: {
        labels: )" << labels.str() << R"(,
        datasets: [
            { label: '上传成功', data: )" << upload_success_data.str() << R"(, borderColor: '#1976d2', tension: 0.3 },
            { label: '下载成功', data: )" << download_success_data.str() << R"(, borderColor: '#388e3c', tension: 0.3 },
            { label: '失败', data: )" << fail_data.str() << R"(, borderColor: '#d32f2f', tension: 0.3 }
        ]
    },
    options: { responsive: true, plugins: { title: { display: true, text: '成功/失败次数' } }, scales: { y: { title: { display: true, text: '累计次数' } } } }
});
</script>
</body>
</html>
)";
}
