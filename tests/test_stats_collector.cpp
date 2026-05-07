#include "test_framework.h"
#include "stats_collector.h"
#include <thread>
#include <vector>

TEST(StatsCollector_InitialValues) {
    StatsCollector stats;
    EXPECT_EQ(stats.upload_success.load(), 0u);
    EXPECT_EQ(stats.upload_fail.load(), 0u);
    EXPECT_EQ(stats.download_success.load(), 0u);
    EXPECT_EQ(stats.download_fail.load(), 0u);
    EXPECT_EQ(stats.upload_bytes.load(), 0u);
    EXPECT_EQ(stats.download_bytes.load(), 0u);
}

TEST(StatsCollector_IncrementCounters) {
    StatsCollector stats;
    stats.upload_success++;
    stats.upload_success++;
    stats.upload_fail++;
    stats.download_success++;
    stats.download_bytes += 1024;

    EXPECT_EQ(stats.upload_success.load(), 2u);
    EXPECT_EQ(stats.upload_fail.load(), 1u);
    EXPECT_EQ(stats.download_success.load(), 1u);
    EXPECT_EQ(stats.download_bytes.load(), 1024u);
}

TEST(StatsCollector_TakeSnapshot) {
    StatsCollector stats;
    stats.upload_success = 10;
    stats.upload_fail = 1;
    stats.download_success = 9;
    stats.download_fail = 2;
    stats.upload_bytes = 10240;
    stats.download_bytes = 9216;

    auto snap = stats.take_snapshot(5);

    EXPECT_EQ(snap.elapsed_seconds, 5);
    EXPECT_EQ(snap.upload_success, 10u);
    EXPECT_EQ(snap.upload_fail, 1u);
    EXPECT_EQ(snap.download_success, 9u);
    EXPECT_EQ(snap.download_fail, 2u);
    EXPECT_EQ(snap.upload_bytes, 10240u);
    EXPECT_EQ(snap.download_bytes, 9216u);
}

TEST(StatsCollector_SnapshotIntervalBytes) {
    StatsCollector stats;
    stats.upload_bytes = 1000;
    stats.download_bytes = 2000;

    auto snap1 = stats.take_snapshot(5);
    EXPECT_EQ(snap1.interval_upload_bytes, 1000u);
    EXPECT_EQ(snap1.interval_download_bytes, 2000u);

    stats.upload_bytes += 500;
    stats.download_bytes += 1000;

    auto snap2 = stats.take_snapshot(10);
    EXPECT_EQ(snap2.interval_upload_bytes, 500u);
    EXPECT_EQ(snap2.interval_download_bytes, 1000u);
    EXPECT_EQ(snap2.upload_bytes, 1500u);
    EXPECT_EQ(snap2.download_bytes, 3000u);
}

TEST(StatsCollector_ConcurrentIncrements) {
    StatsCollector stats;
    const int thread_count = 10;
    const int increments_per_thread = 1000;

    std::vector<std::thread> threads;
    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&stats]() {
            for (int i = 0; i < increments_per_thread; ++i) {
                stats.upload_success++;
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(stats.upload_success.load(),
              static_cast<uint64_t>(thread_count * increments_per_thread));
}

RUN_ALL_TESTS()
