#include "stats_collector.h"

Snapshot StatsCollector::take_snapshot(int elapsed_seconds) {
    uint64_t cur_up = upload_bytes.load();
    uint64_t cur_down = download_bytes.load();

    Snapshot snap;
    snap.elapsed_seconds = elapsed_seconds;
    snap.upload_bytes = cur_up;
    snap.download_bytes = cur_down;
    snap.upload_success = upload_success.load();
    snap.upload_fail = upload_fail.load();
    snap.download_success = download_success.load();
    snap.download_fail = download_fail.load();
    snap.interval_upload_bytes = cur_up - last_upload_bytes_;
    snap.interval_download_bytes = cur_down - last_download_bytes_;

    last_upload_bytes_ = cur_up;
    last_download_bytes_ = cur_down;

    return snap;
}
