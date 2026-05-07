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
