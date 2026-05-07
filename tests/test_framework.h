#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <functional>

struct TestCase {
    std::string name;
    std::function<void()> func;
};

static std::vector<TestCase>& get_tests() {
    static std::vector<TestCase> tests;
    return tests;
}

static int& fail_count() {
    static int c = 0;
    return c;
}

static int& current_line() {
    static int l = 0;
    return l;
}

#define TEST(name) \
    static void test_##name(); \
    static bool reg_##name = (get_tests().push_back({#name, test_##name}), true); \
    static void test_##name()

#define EXPECT_EQ(a, b) do { \
    auto _a = (a); auto _b = (b); \
    if (!(_a == _b)) { \
        std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__ << " - " << #a << " == " << #b \
                  << " (got " << _a << " vs " << _b << ")" << std::endl; \
        fail_count()++; \
    } \
} while(0)

#define EXPECT_NE(a, b) do { \
    auto _a = (a); auto _b = (b); \
    if (!(_a != _b)) { \
        std::cerr << "  FAIL: " << __FILE__ << ":" << __LINE__ << " - " << #a << " != " << #b << std::endl; \
        fail_count()++; \
    } \
} while(0)

#define RUN_ALL_TESTS() \
    int main() { \
        int passed = 0; \
        for (auto& t : get_tests()) { \
            std::cout << "[ RUN  ] " << t.name << std::endl; \
            int fails_before = fail_count(); \
            t.func(); \
            if (fail_count() == fails_before) { \
                std::cout << "[ PASS ] " << t.name << std::endl; \
                passed++; \
            } else { \
                std::cout << "[ FAIL ] " << t.name << std::endl; \
            } \
        } \
        int total = get_tests().size(); \
        std::cout << "\n" << passed << "/" << total << " tests passed."; \
        if (fail_count() > 0) std::cout << " " << fail_count() << " assertion(s) failed."; \
        std::cout << std::endl; \
        return fail_count() > 0 ? 1 : 0; \
    }
