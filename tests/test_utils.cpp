#include "test_framework.h"
#include "utils.h"

TEST(ParseFileSize_PlainNumber) {
    EXPECT_EQ(parse_file_size("100"), 100u);
    EXPECT_EQ(parse_file_size("0"), 0u);
    EXPECT_EQ(parse_file_size("1048576"), 1048576u);
}

TEST(ParseFileSize_Kilobytes) {
    EXPECT_EQ(parse_file_size("512K"), 512u * 1024);
    EXPECT_EQ(parse_file_size("1K"), 1024u);
    EXPECT_EQ(parse_file_size("100k"), 100u * 1024);
}

TEST(ParseFileSize_Megabytes) {
    EXPECT_EQ(parse_file_size("1M"), 1024u * 1024);
    EXPECT_EQ(parse_file_size("10M"), 10u * 1024 * 1024);
    EXPECT_EQ(parse_file_size("5m"), 5u * 1024 * 1024);
}

TEST(ParseFileSize_Gigabytes) {
    EXPECT_EQ(parse_file_size("1G"), 1u * 1024 * 1024 * 1024);
    EXPECT_EQ(parse_file_size("2g"), 2u * 1024 * 1024 * 1024);
}

TEST(ParseFileSize_InvalidInput) {
    EXPECT_EQ(parse_file_size(""), 0u);
    EXPECT_EQ(parse_file_size("abc"), 0u);
    EXPECT_EQ(parse_file_size("K"), 0u);
}

TEST(GenerateRandomData_CorrectSize) {
    auto data = generate_random_data(1024);
    EXPECT_EQ(data.size(), 1024u);
}

TEST(GenerateRandomData_ZeroSize) {
    auto data = generate_random_data(0);
    EXPECT_EQ(data.size(), 0u);
}

TEST(FormatElapsed_Formatting) {
    EXPECT_EQ(format_elapsed(0), std::string("00:00"));
    EXPECT_EQ(format_elapsed(5), std::string("00:05"));
    EXPECT_EQ(format_elapsed(65), std::string("01:05"));
    EXPECT_EQ(format_elapsed(3661), std::string("61:01"));
}

RUN_ALL_TESTS()
