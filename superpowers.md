# FTP 压力测试工具 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 构建一个跨平台（Linux/Windows）的 FTP 压力测试工具，支持多线程并发上传下载，带实时统计和 HTML 可视化报告。

**Architecture:** 多线程架构，每线程独立 libcurl easy 句柄，通过 CLI 参数控制线程数、时长、文件大小。主线程负责计时和统计采集，工作线程循环执行连接→上传→下载→清理。使用 `std::atomic` 保证统计数据的线程安全。

**Tech Stack:** C++17, libcurl, CMake, Google Test

---

## File Structure

| File | Responsibility |
|------|---------------|
| `CMakeLists.txt` | 项目构建配置，链接 libcurl 和 pthread |
| `src/utils.h` | `parse_file_size()`, `generate_random_data()`, `format_elapsed()` 声明 |
| `src/utils.cpp` | 工具函数实现 |
| `src/config.h` | `Config` 结构体和 `parse_args()` 声明 |
| `src/config.cpp` | CLI 参数解析与校验 |
| `src/stats_collector.h` | `StatsCollector` 类和 `Snapshot` 结构体声明 |
| `src/stats_collector.cpp` | 原子计数器 + 快照采样实现 |
| `src/worker.h` | `Worker` 类声明 |
| `src/worker.cpp` | FTP 上传下载工作循环 |
| `src/reporter.h` | `Reporter` 类声明 |
| `src/reporter.cpp` | 终端实时输出 + HTML 报告生成 |
| `src/main.cpp` | 程序入口，组装所有模块 |
| `tests/CMakeLists.txt` | 测试构建配置 |
| `tests/test_utils.cpp` | utils 模块单元测试 |
| `tests/test_config.cpp` | config 模块单元测试 |
| `tests/test_stats_collector.cpp` | StatsCollector 单元测试 |

---

### Task 1: 项目骨架与 CMake 配置

**Files:**
- Create: `CMakeLists.txt`
- Create: `tests/CMakeLists.txt`

- [ ] **Step 1: 编写主 CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.14)
project(ftp_stress_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

add_executable(ftp_stress_test
    src/main.cpp
    src/config.cpp
    src/worker.cpp
    src/stats_collector.cpp
    src/reporter.cpp
    src/utils.cpp
)

target_include_directories(ftp_stress_test PRIVATE src ${CURL_INCLUDE_DIRS})
target_link_libraries(ftp_stress_test PRIVATE ${CURL_LIBRARIES})

if(UNIX)
    target_link_libraries(ftp_stress_test PRIVATE pthread)
endif()

# Testing
enable_testing()
add_subdirectory(tests)
```

- [ ] **Step 2: 编写测试 CMakeLists.txt**

```cmake
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(ftp_stress_test_tests
    test_utils.cpp
    test_config.cpp
    test_stats_collector.cpp
    ../src/utils.cpp
    ../src/config.cpp
    ../src/stats_collector.cpp
)

target_include_directories(ftp_stress_test_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CURL_INCLUDE_DIRS})
target_link_libraries(ftp_stress_test_tests PRIVATE GTest::gtest_main ${CURL_LIBRARIES})

if(UNIX)
    target_link_libraries(ftp_stress_test_tests PRIVATE pthread)
endif()

include(GoogleTest)
gtest_discover_tests(ftp_stress_test_tests)
```

- [ ] **Step 3: 创建源文件占位，验证构建**

创建所有源文件的空壳（仅含必要的空函数或最简 main），确保 CMake 配置正确：

`src/main.cpp`:
```cpp
int main() { return 0; }
```

`src/utils.h`:
```cpp
#pragma once
#include <cstdint>
#include <vector>
#include <string>
```

`src/utils.cpp`:
```cpp
#include "utils.h"
```

`src/config.h`:
```cpp
#pragma once
#include <string>
#include <cstdint>

struct Config {
    std::string host = "";
    uint16_t port = 21;
    std::string username = "anonymous";
    std::string password = "";
    int threads = 1;
    int duration = 60;
    uint64_t file_size = 1024 * 1024;
    std::string report_path = "";
    int interval = 5;
    int timeout = 300;
};

Config parse_args(int argc, char* argv[]);
```

`src/config.cpp`:
```cpp
#include "config.h"
```

`src/stats_collector.h`:
```cpp
#pragma once
```

`src/stats_collector.cpp`:
```cpp
#include "stats_collector.h"
```

`src/worker.h`:
```cpp
#pragma once
```

`src/worker.cpp`:
```cpp
#include "worker.h"
```

`src/reporter.h`:
```cpp
#pragma once
```

`src/reporter.cpp`:
```cpp
#include "reporter.h"
```

- [ ] **Step 4: 构建验证**

Run: `cmake -B build -S . && cmake --build build`
Expected: 编译成功，无错误

- [ ] **Step 5: Commit**

```bash
git add CMakeLists.txt tests/CMakeLists.txt src/
git commit -m "feat: project skeleton with CMake and empty source files"
```

---

### Task 2: Utils 模块 — 文件大小解析与随机数据生成

**Files:**
- Modify: `src/utils.h`
- Modify: `src/utils.cpp`
- Create: `tests/test_utils.cpp`

- [ ] **Step 1: 编写 utils 测试**

`tests/test_utils.cpp`:
```cpp
#include <gtest/gtest.h>
#include "utils.h"

TEST(ParseFileSize, PlainNumber) {
    EXPECT_EQ(parse_file_size("100"), 100u);
    EXPECT_EQ(parse_file_size("0"), 0u);
    EXPECT_EQ(parse_file_size("1048576"), 1048576u);
}

TEST(ParseFileSize, Kilobytes) {
    EXPECT_EQ(parse_file_size("512K"), 512u * 1024);
    EXPECT_EQ(parse_file_size("1K"), 1024u);
    EXPECT_EQ(parse_file_size("100k"), 100u * 1024);
}

TEST(ParseFileSize, Megabytes) {
    EXPECT_EQ(parse_file_size("1M"), 1024u * 1024);
    EXPECT_EQ(parse_file_size("10M"), 10u * 1024 * 1024);
    EXPECT_EQ(parse_file_size("5m"), 5u * 1024 * 1024);
}

TEST(ParseFileSize, Gigabytes) {
    EXPECT_EQ(parse_file_size("1G"), 1u * 1024 * 1024 * 1024);
    EXPECT_EQ(parse_file_size("2g"), 2u * 1024 * 1024 * 1024);
}

TEST(ParseFileSize, InvalidInput) {
    EXPECT_EQ(parse_file_size(""), 0u);
    EXPECT_EQ(parse_file_size("abc"), 0u);
    EXPECT_EQ(parse_file_size("K"), 0u);
}

TEST(GenerateRandomData, CorrectSize) {
    auto data = generate_random_data(1024);
    EXPECT_EQ(data.size(), 1024u);
}

TEST(GenerateRandomData, ZeroSize) {
    auto data = generate_random_data(0);
    EXPECT_EQ(data.size(), 0u);
}

TEST(FormatElapsed, Formatting) {
    EXPECT_EQ(format_elapsed(0), "00:00");
    EXPECT_EQ(format_elapsed(5), "00:05");
    EXPECT_EQ(format_elapsed(65), "01:05");
    EXPECT_EQ(format_elapsed(3661), "61:01");
}
```

- [ ] **Step 2: 运行测试验证失败**

Run: `cmake --build build && cd build && ./tests/ftp_stress_test_tests --gtest_filter='ParseFileSize*'`
Expected: 编译失败或测试失败（函数未定义）

- [ ] **Step 3: 实现 utils 模块**

`src/utils.h`:
```cpp
#pragma once
#include <cstdint>
#include <string>
#include <vector>

uint64_t parse_file_size(const std::string& input);
std::vector<uint8_t> generate_random_data(uint64_t size);
std::string format_elapsed(int seconds);
```

`src/utils.cpp`:
```cpp
#include "utils.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cstdlib>

uint64_t parse_file_size(const std::string& input) {
    if (input.empty()) return 0;

    char suffix = '\0';
    std::string num_part;

    char last = static_cast<char>(std::tolower(input.back()));
    if (last == 'k' || last == 'm' || last == 'g') {
        suffix = last;
        num_part = input.substr(0, input.size() - 1);
    } else {
        num_part = input;
    }

    if (num_part.empty()) return 0;

    for (char c : num_part) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return 0;
    }

    uint64_t value = std::stoull(num_part);

    switch (suffix) {
        case 'k': return value * 1024;
        case 'm': return value * 1024 * 1024;
        case 'g': return value * 1024 * 1024 * 1024;
        default:  return value;
    }
}

std::vector<uint8_t> generate_random_data(uint64_t size) {
    std::vector<uint8_t> data(size);
    if (size == 0) return data;

    std::mt19937 gen(42);
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    for (auto& byte : data) {
        byte = dist(gen);
    }
    return data;
}

std::string format_elapsed(int seconds) {
    int min = seconds / 60;
    int sec = seconds % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << min << ":"
        << std::setw(2) << std::setfill('0') << sec;
    return oss.str();
}
```

- [ ] **Step 4: 运行测试验证通过**

Run: `cmake --build build && cd build && ./tests/ftp_stress_test_tests --gtest_filter='ParseFileSize*:GenerateRandomData*:FormatElapsed*'`
Expected: 所有测试 PASS

- [ ] **Step 5: Commit**

```bash
git add src/utils.h src/utils.cpp tests/test_utils.cpp
git commit -m "feat: utils module with file size parsing, random data, time formatting"
```

---

### Task 3: Config 模块 — CLI 参数解析

**Files:**
- Modify: `src/config.h`
- Modify: `src/config.cpp`
- Create: `tests/test_config.cpp`

- [ ] **Step 1: 编写 config 测试**

`tests/test_config.cpp`:
```cpp
#include <gtest/gtest.h>
#include "config.h"

TEST(ParseArgs, DefaultValues) {
    const char* argv[] = {"ftp_stress_test", "--host", "192.168.1.1"};
    Config cfg = parse_args(3, const_cast<char**>(argv));
    EXPECT_EQ(cfg.host, "192.168.1.1");
    EXPECT_EQ(cfg.port, 21);
    EXPECT_EQ(cfg.username, "anonymous");
    EXPECT_EQ(cfg.password, "");
    EXPECT_EQ(cfg.threads, 1);
    EXPECT_EQ(cfg.duration, 60);
    EXPECT_EQ(cfg.file_size, 1024u * 1024);
    EXPECT_EQ(cfg.interval, 5);
    EXPECT_EQ(cfg.timeout, 300);
}

TEST(ParseArgs, AllOptions) {
    const char* argv[] = {
        "ftp_stress_test",
        "--host", "10.0.0.1",
        "--port", "2121",
        "--user", "admin",
        "--pass", "secret",
        "--threads", "8",
        "--duration", "120",
        "--file-size", "5M",
        "--interval", "10",
        "--timeout", "60",
        "--report", "my_report.html"
    };
    Config cfg = parse_args(21, const_cast<char**>(argv));
    EXPECT_EQ(cfg.host, "10.0.0.1");
    EXPECT_EQ(cfg.port, 2121);
    EXPECT_EQ(cfg.username, "admin");
    EXPECT_EQ(cfg.password, "secret");
    EXPECT_EQ(cfg.threads, 8);
    EXPECT_EQ(cfg.duration, 120);
    EXPECT_EQ(cfg.file_size, 5u * 1024 * 1024);
    EXPECT_EQ(cfg.interval, 10);
    EXPECT_EQ(cfg.timeout, 60);
    EXPECT_EQ(cfg.report_path, "my_report.html");
}

TEST(ParseArgs, MissingHostExits) {
    const char* argv[] = {"ftp_stress_test"};
    EXPECT_EXIT(parse_args(1, const_cast<char**>(argv)),
                testing::ExitedWithCode(1), ".*");
}

TEST(ParseArgs, InvalidThreadsExits) {
    const char* argv[] = {"ftp_stress_test", "--host", "1.2.3.4", "--threads", "0"};
    EXPECT_EXIT(parse_args(5, const_cast<char**>(argv)),
                testing::ExitedWithCode(1), ".*");
}

TEST(ParseArgs, InvalidDurationExits) {
    const char* argv[] = {"ftp_stress_test", "--host", "1.2.3.4", "--duration", "-1"};
    EXPECT_EXIT(parse_args(5, const_cast<char**>(argv)),
                testing::ExitedWithCode(1), ".*");
}
```

- [ ] **Step 2: 运行测试验证失败**

Run: `cmake --build build && cd build && ./tests/ftp_stress_test_tests --gtest_filter='ParseArgs*'`
Expected: 链接失败或测试失败

- [ ] **Step 3: 实现 config 模块**

`src/config.h`:
```cpp
#pragma once
#include <string>
#include <cstdint>

struct Config {
    std::string host = "";
    uint16_t port = 21;
    std::string username = "anonymous";
    std::string password = "";
    int threads = 1;
    int duration = 60;
    uint64_t file_size = 1024 * 1024;
    std::string report_path = "";
    int interval = 5;
    int timeout = 300;
};

Config parse_args(int argc, char* argv[]);
```

`src/config.cpp`:
```cpp
#include "config.h"
#include "utils.h"
#include <iostream>
#include <cstdlib>
#include <string>

static void print_usage_and_exit(const std::string& error = "") {
    if (!error.empty()) {
        std::cerr << "Error: " << error << "\n\n";
    }
    std::cerr << "Usage: ftp_stress_test [options]\n\n"
              << "Required:\n"
              << "  --host <addr>          FTP server address\n\n"
              << "Optional:\n"
              << "  --port <port>          FTP port (default: 21)\n"
              << "  --user <username>      FTP username (default: anonymous)\n"
              << "  --pass <password>      FTP password (default: empty)\n"
              << "  --threads <n>          Concurrent threads (default: 1)\n"
              << "  --duration <seconds>   Test duration in seconds (default: 60)\n"
              << "  --file-size <size>     Test file size, supports K/M/G suffix (default: 1M)\n"
              << "  --report <path>        Report output path (default: report_<timestamp>.html)\n"
              << "  --interval <seconds>   Stats output interval (default: 5)\n"
              << "  --timeout <seconds>    Per-transfer timeout (default: 300)\n";
    std::exit(1);
}

Config parse_args(int argc, char* argv[]) {
    Config cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--host" && i + 1 < argc) {
            cfg.host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            cfg.port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--user" && i + 1 < argc) {
            cfg.username = argv[++i];
        } else if (arg == "--pass" && i + 1 < argc) {
            cfg.password = argv[++i];
        } else if (arg == "--threads" && i + 1 < argc) {
            cfg.threads = std::stoi(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            cfg.duration = std::stoi(argv[++i]);
        } else if (arg == "--file-size" && i + 1 < argc) {
            cfg.file_size = parse_file_size(argv[++i]);
        } else if (arg == "--report" && i + 1 < argc) {
            cfg.report_path = argv[++i];
        } else if (arg == "--interval" && i + 1 < argc) {
            cfg.interval = std::stoi(argv[++i]);
        } else if (arg == "--timeout" && i + 1 < argc) {
            cfg.timeout = std::stoi(argv[++i]);
        } else {
            print_usage_and_exit("Unknown option: " + arg);
        }
    }

    if (cfg.host.empty()) {
        print_usage_and_exit("--host is required");
    }
    if (cfg.threads <= 0) {
        print_usage_and_exit("--threads must be > 0");
    }
    if (cfg.duration <= 0) {
        print_usage_and_exit("--duration must be > 0");
    }
    if (cfg.file_size == 0) {
        print_usage_and_exit("--file-size must be > 0");
    }
    if (cfg.interval <= 0) {
        print_usage_and_exit("--interval must be > 0");
    }
    if (cfg.timeout <= 0) {
        print_usage_and_exit("--timeout must be > 0");
    }

    return cfg;
}
```

- [ ] **Step 4: 运行测试验证通过**

Run: `cmake --build build && cd build && ./tests/ftp_stress_test_tests --gtest_filter='ParseArgs*'`
Expected: 所有测试 PASS

- [ ] **Step 5: Commit**

```bash
git add src/config.h src/config.cpp tests/test_config.cpp
git commit -m "feat: config module with CLI argument parsing and validation"
```

---

### Task 4: StatsCollector 模块 — 原子统计与快照

**Files:**
- Modify: `src/stats_collector.h`
- Modify: `src/stats_collector.cpp`
- Create: `tests/test_stats_collector.cpp`

- [ ] **Step 1: 编写 stats_collector 测试**

`tests/test_stats_collector.cpp`:
```cpp
#include <gtest/gtest.h>
#include "stats_collector.h"
#include <thread>
#include <vector>

TEST(StatsCollector, InitialValues) {
    StatsCollector stats;
    EXPECT_EQ(stats.upload_success.load(), 0u);
    EXPECT_EQ(stats.upload_fail.load(), 0u);
    EXPECT_EQ(stats.download_success.load(), 0u);
    EXPECT_EQ(stats.download_fail.load(), 0u);
    EXPECT_EQ(stats.upload_bytes.load(), 0u);
    EXPECT_EQ(stats.download_bytes.load(), 0u);
}

TEST(StatsCollector, IncrementCounters) {
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

TEST(StatsCollector, TakeSnapshot) {
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

TEST(StatsCollector, SnapshotComputesIntervalBytes) {
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

TEST(StatsCollector, ConcurrentIncrements) {
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
```

- [ ] **Step 2: 运行测试验证失败**

Run: `cmake --build build && cd build && ./tests/ftp_stress_test_tests --gtest_filter='StatsCollector*'`
Expected: 编译失败

- [ ] **Step 3: 实现 stats_collector 模块**

`src/stats_collector.h`:
```cpp
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
```

`src/stats_collector.cpp`:
```cpp
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
```

- [ ] **Step 4: 运行测试验证通过**

Run: `cmake --build build && cd build && ./tests/ftp_stress_test_tests --gtest_filter='StatsCollector*'`
Expected: 所有测试 PASS

- [ ] **Step 5: Commit**

```bash
git add src/stats_collector.h src/stats_collector.cpp tests/test_stats_collector.cpp
git commit -m "feat: StatsCollector with atomic counters and snapshot sampling"
```

---

### Task 5: Worker 模块 — FTP 上传下载工作循环

**Files:**
- Modify: `src/worker.h`
- Modify: `src/worker.cpp`

此模块依赖 libcurl 进行实际 FTP 操作，不适合用 mock 做单元测试，通过集成测试验证。此任务只编写实现代码。

- [ ] **Step 1: 实现 worker 模块**

`src/worker.h`:
```cpp
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
```

`src/worker.cpp`:
```cpp
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

        delete_remote_file(curl);
        curl_easy_cleanup(curl);

        if (!upload_ok) {
            long response_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            // Authentication failure (530) is fatal
            if (response_code == 530) {
                stop_flag_.store(true);
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
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
    curl_easy_setopt(c, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_PASV);

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
    curl_easy_setopt(c, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_PASV);

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
```

- [ ] **Step 2: 构建验证**

Run: `cmake --build build`
Expected: 编译成功，无错误

- [ ] **Step 3: Commit**

```bash
git add src/worker.h src/worker.cpp
git commit -m "feat: Worker module with FTP upload/download/delete cycle"
```

---

### Task 6: Reporter 模块 — 终端输出与 HTML 报告

**Files:**
- Modify: `src/reporter.h`
- Modify: `src/reporter.cpp`

- [ ] **Step 1: 实现 reporter 模块**

`src/reporter.h`:
```cpp
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
    std::string format_bytes(uint64_t bytes);
    std::string format_speed(uint64_t bytes, int seconds);

    const Config& cfg_;
    const StatsCollector& stats_;
    int thread_count_;
};
```

`src/reporter.cpp`:
```cpp
#include "reporter.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>

Reporter::Reporter(const Config& cfg, const StatsCollector& stats, int thread_count)
    : cfg_(cfg), stats_(stats), thread_count_(thread_count) {}

std::string Reporter::format_bytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_idx = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && unit_idx < 4) {
        size /= 1024.0;
        unit_idx++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_idx];
    return oss.str();
}

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

    // Build JSON arrays for Chart.js
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
```

- [ ] **Step 2: 构建验证**

Run: `cmake --build build`
Expected: 编译成功

- [ ] **Step 3: Commit**

```bash
git add src/reporter.h src/reporter.cpp
git commit -m "feat: Reporter module with terminal output and HTML chart report"
```

---

### Task 7: Main 入口 — 组装所有模块

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: 实现 main.cpp**

```cpp
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

static std::atomic<bool> g_stop_flag{false};

static void signal_handler(int) {
    g_stop_flag.store(true);
}

static void setup_signal_handler() {
#ifndef _WIN32
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
#else
    signal(SIGINT, signal_handler);
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

    // Start workers
    std::vector<std::unique_ptr<Worker>> workers;
    for (int i = 0; i < cfg.threads; ++i) {
        workers.push_back(std::make_unique<Worker>(i, cfg, test_data, stats, g_stop_flag));
    }
    for (auto& w : workers) w->start();

    // Main loop: collect snapshots and print realtime stats
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

    // Wait for workers
    for (auto& w : workers) w->join();

    auto end_time = std::chrono::steady_clock::now();
    double total_seconds = std::chrono::duration<double>(end_time - start_time).count();

    // Generate reports
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
```

- [ ] **Step 2: 构建并验证**

Run: `cmake --build build`
Expected: 编译成功

- [ ] **Step 3: 运行帮助验证参数解析**

Run: `./build/ftp_stress_test`
Expected: 打印用法说明并退出

- [ ] **Step 4: Commit**

```bash
git add src/main.cpp
git commit -m "feat: main entry point assembling all modules"
```

---

### Task 8: 跨平台信号处理完善

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: 完善跨平台 Ctrl+C 处理**

更新 `main.cpp` 中的 `setup_signal_handler()` 函数，在 Windows 上使用 `SetConsoleCtrlHandler`：

```cpp
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
```

- [ ] **Step 2: 构建验证**

Run: `cmake --build build`
Expected: 编译成功

- [ ] **Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "feat: cross-platform Ctrl+C handling with Windows SetConsoleCtrlHandler"
```

---

### Task 9: 全量构建与运行所有测试

**Files:** 无新文件

- [ ] **Step 1: 完整重新构建**

Run: `cmake -B build -S . && cmake --build build`
Expected: 零警告零错误

- [ ] **Step 2: 运行全部单元测试**

Run: `cd build && ./tests/ftp_stress_test_tests`
Expected: 所有测试 PASS

- [ ] **Step 3: 手动集成测试（需 FTP 服务器）**

Run: `./build/ftp_stress_test --host <your_ftp_server> --user <username> --pass <password> --threads 2 --duration 10 --file-size 100K --interval 2`
Expected:
- 终端输出实时统计行
- 10 秒后测试结束
- 生成 HTML 报告文件，浏览器打开可见折线图

- [ ] **Step 4: Final commit**

```bash
git add -A
git commit -m "chore: verify full build and all tests pass"
```

