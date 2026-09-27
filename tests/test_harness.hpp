#ifndef BHARATOPT_TEST_HARNESS_HPP
#define BHARATOPT_TEST_HARNESS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <functional>

namespace bharatopt::testing {

struct TestResult {
    std::string name;
    bool passed;
    std::string failure_message;
};

class TestRunner {
public:
    static TestRunner& instance() {
        static TestRunner runner;
        return runner;
    }

    void add_test(const std::string& name, std::function<void()> test_func) {
        tests_.push_back({name, test_func});
    }

    int run_all() {
        int passed_count = 0;
        int failed_count = 0;
        std::cout << "========================================================\n";
        std::cout << " Running BharatOpt Unit Test Suite\n";
        std::cout << "========================================================\n\n";

        for (const auto& test : tests_) {
            current_failed_ = false;
            current_failure_msg_ = "";
            try {
                test.func();
                if (!current_failed_) {
                    std::cout << "[ PASS ] " << test.name << std::endl;
                    passed_count++;
                } else {
                    std::cout << "[ FAIL ] " << test.name << " - " << current_failure_msg_ << std::endl;
                    failed_count++;
                }
            } catch (const std::exception& e) {
                std::cout << "[ CRASH ] " << test.name << " - Exception: " << e.what() << std::endl;
                failed_count++;
            } catch (...) {
                std::cout << "[ CRASH ] " << test.name << " - Unknown Exception" << std::endl;
                failed_count++;
            }
        }

        std::cout << "\n--------------------------------------------------------\n";
        std::cout << " Test Summary: " << passed_count << " Passed, " << failed_count << " Failed.\n";
        std::cout << "========================================================\n" << std::flush;
        return (failed_count == 0) ? 0 : 1;
    }

    void record_failure(const std::string& msg) {
        current_failed_ = true;
        current_failure_msg_ = msg;
    }

private:
    struct TestItem {
        std::string name;
        std::function<void()> func;
    };
    std::vector<TestItem> tests_;
    bool current_failed_{false};
    std::string current_failure_msg_{""};
};

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> func) {
        TestRunner::instance().add_test(name, func);
    }
};

} // namespace bharatopt::testing

#define TEST_CASE(test_name) \
    void test_func_##test_name(); \
    static bharatopt::testing::TestRegistrar registrar_##test_name(#test_name, test_func_##test_name); \
    void test_func_##test_name()

#define EXPECT_TRUE(cond) \
    if (!(cond)) { \
        bharatopt::testing::TestRunner::instance().record_failure("EXPECT_TRUE failed: " #cond); \
        return; \
    }

#define EXPECT_FALSE(cond) \
    if (cond) { \
        bharatopt::testing::TestRunner::instance().record_failure("EXPECT_FALSE failed: " #cond); \
        return; \
    }

#define EXPECT_EQ(val1, val2) \
    if ((val1) != (val2)) { \
        bharatopt::testing::TestRunner::instance().record_failure("EXPECT_EQ failed: " #val1 " != " #val2); \
        return; \
    }

#define EXPECT_NEAR(val1, val2, tol) \
    if (std::abs((val1) - (val2)) > (tol)) { \
        bharatopt::testing::TestRunner::instance().record_failure("EXPECT_NEAR failed: |" #val1 " - " #val2 "| > " #tol); \
        return; \
    }

#endif // BHARATOPT_TEST_HARNESS_HPP
