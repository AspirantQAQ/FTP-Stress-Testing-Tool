#include "test_framework.h"
#include "config.h"
#include <cstdlib>

// Override exit to test error cases
static std::string last_error;
static bool expect_exit = false;
static bool exited = false;

TEST(ParseArgs_DefaultValues) {
    const char* argv[] = {"ftp_stress_test", "--host", "192.168.1.1"};
    Config cfg = parse_args(3, const_cast<char**>(argv));
    EXPECT_EQ(cfg.host, std::string("192.168.1.1"));
    EXPECT_EQ(cfg.port, 21);
    EXPECT_EQ(cfg.username, std::string("anonymous"));
    EXPECT_EQ(cfg.password, std::string(""));
    EXPECT_EQ(cfg.threads, 1);
    EXPECT_EQ(cfg.duration, 60);
    EXPECT_EQ(cfg.file_size, 1024u * 1024);
    EXPECT_EQ(cfg.interval, 5);
    EXPECT_EQ(cfg.timeout, 300);
}

TEST(ParseArgs_AllOptions) {
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
    EXPECT_EQ(cfg.host, std::string("10.0.0.1"));
    EXPECT_EQ(cfg.port, 2121);
    EXPECT_EQ(cfg.username, std::string("admin"));
    EXPECT_EQ(cfg.password, std::string("secret"));
    EXPECT_EQ(cfg.threads, 8);
    EXPECT_EQ(cfg.duration, 120);
    EXPECT_EQ(cfg.file_size, 5u * 1024 * 1024);
    EXPECT_EQ(cfg.interval, 10);
    EXPECT_EQ(cfg.timeout, 60);
    EXPECT_EQ(cfg.report_path, std::string("my_report.html"));
}

// Note: exit-based tests omitted in this lightweight framework
// Missing host, invalid threads, invalid duration are validated at runtime

RUN_ALL_TESTS()
