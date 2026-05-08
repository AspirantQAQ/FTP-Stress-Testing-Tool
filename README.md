# FTP Stress Testing Tool

一个基于 C++17 的多线程 FTP 服务器压力测试工具，支持自定义并发线程数、文件大小和测试时长，并自动生成包含图表的 HTML 测试报告。

## 功能特性

- **多线程并发** — 可配置线程数，同时发起多个 FTP 连接
- **上传/下载测试** — 每个线程循环执行上传 → 下载 → 删除，持续压测
- **实时统计** — 终端按间隔输出上传/下载成功数、失败数和传输速率
- **HTML 报告** — 测试结束后自动生成可视化报告（传输速率曲线 + 成功/失败次数）
- **灵活配置** — 支持命令行参数自定义服务器地址、端口、用户名、文件大小等
- **跨平台** — 支持 Linux 和 Windows

## 依赖

- C++17 编译器（GCC 7+ / Clang 5+ / MSVC 2017+）
- CMake >= 3.14
- libcurl 开发库

### 安装依赖（Ubuntu/Debian）

```bash
sudo apt install cmake build-essential libcurl4-openssl-dev
```

## 编译

```bash
git clone https://github.com/AspirantQAQ/FTP-Stress-Testing-Tool.git
cd FTP-Stress-Testing-Tool
mkdir build && cd build
cmake ..
make
```

## 使用方法

```bash
./ftp_stress_test --host <FTP服务器地址> [选项]
```

### 参数说明

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `--host <addr>` | FTP 服务器地址（必填） | — |
| `--port <port>` | FTP 端口 | 21 |
| `--user <username>` | FTP 用户名 | anonymous |
| `--pass <password>` | FTP 密码 | 空 |
| `--threads <n>` | 并发线程数 | 1 |
| `--duration <seconds>` | 测试持续时间（秒） | 60 |
| `--file-size <size>` | 测试文件大小，支持 K/M/G 后缀 | 1M |
| `--report <path>` | 报告输出路径 | report_时间戳.html |
| `--interval <seconds>` | 统计输出间隔（秒） | 5 |
| `--timeout <seconds>` | 单次传输超时（秒） | 300 |

### 示例

**基础测试：**

```bash
./ftp_stress_test --host 192.168.1.100
```

**10 线程、5 分钟、100MB 文件：**

```bash
./ftp_stress_test --host 192.168.1.100 --threads 10 --duration 300 --file-size 100M
```

**带认证 + 自定义报告路径：**

```bash
./ftp_stress_test --host ftp.example.com --port 2121 \
  --user admin --pass secret \
  --threads 20 --duration 120 --file-size 50M \
  --report my_report.html
```

## 输出示例

### 终端实时输出

```
FTP Stress Test Starting...
  Server: 192.168.1.100:21
  Threads: 10
  Duration: 60s
  File size: 1.00 MB

[00:05] Threads: 10 | Upload:    48 success,   2 fail | Download:    45 success,   3 fail | Speed: 12.35 MB/s
[00:10] Threads: 10 | Upload:   102 success,   3 fail | Download:    98 success,   5 fail | Speed: 13.21 MB/s
...
Test completed in 60 seconds.
Report saved to: report_20260508_143000.html
```

### HTML 报告

测试结束后自动生成 HTML 报告，包含：

- 测试配置概览
- 上传成功 / 下载成功 / 总失败 / 平均速率统计卡片
- 传输速率随时间变化的折线图（上传/下载/合计）
- 成功/失败次数累计折线图

## 项目结构

```
.
├── CMakeLists.txt
├── src/
│   ├── main.cpp            # 程序入口
│   ├── config.h/cpp        # 命令行参数解析
│   ├── worker.h/cpp        # 多线程 FTP 工作线程
│   ├── stats_collector.h/cpp # 原子操作的统计收集器
│   ├── reporter.h/cpp      # 实时输出与 HTML 报告生成
│   └── utils.h/cpp         # 工具函数
└── tests/
    ├── CMakeLists.txt
    ├── test_framework.h
    ├── test_config.cpp
    ├── test_utils.cpp
    └── test_stats_collector.cpp
```

## 许可证

MIT License
