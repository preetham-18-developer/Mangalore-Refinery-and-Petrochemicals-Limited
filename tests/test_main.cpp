#include "test_harness.hpp"
#include <bharatopt/version.hpp>
#include <bharatopt/config.hpp>

TEST_CASE(VersionInfoTest) {
    std::string version = bharatopt::get_version_info();
    EXPECT_FALSE(version.empty());
    EXPECT_TRUE(version.find("BharatOpt") != std::string::npos);
}

TEST_CASE(ConfigurationTolerancesTest) {
    EXPECT_NEAR(bharatopt::DEFAULT_FEASIBILITY_TOLERANCE, 1e-7, 1e-12);
    EXPECT_NEAR(bharatopt::DEFAULT_OPTIMALITY_TOLERANCE, 1e-7, 1e-12);
    EXPECT_TRUE(bharatopt::BHARATOPT_INFINITY > 1e20);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return bharatopt::testing::TestRunner::instance().run_all();
}
