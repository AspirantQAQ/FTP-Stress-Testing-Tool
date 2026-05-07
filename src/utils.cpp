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

std::string format_bytes(uint64_t bytes) {
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
