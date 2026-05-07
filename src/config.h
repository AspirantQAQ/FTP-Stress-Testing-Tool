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
