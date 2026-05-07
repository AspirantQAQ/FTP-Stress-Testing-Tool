#pragma once
#include <cstdint>
#include <string>
#include <vector>

uint64_t parse_file_size(const std::string& input);
std::vector<uint8_t> generate_random_data(uint64_t size);
std::string format_elapsed(int seconds);
std::string format_bytes(uint64_t bytes);
