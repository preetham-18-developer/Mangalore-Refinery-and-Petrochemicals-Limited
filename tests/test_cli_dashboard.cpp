#include <bharatopt/cli_dashboard.hpp>
#include <bharatopt/final_benchmark.hpp>
#include "test_harness.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>

namespace bharatopt {

// Helper to capture stdout/stderr output during dashboard rendering
class OutputCapture {
    std::stringstream buffer_;
    std::streambuf* old_cout_;
    std::streambuf* old_cerr_;
public:
    OutputCapture() {
        old_cout_ = std::cout.rdbuf(buffer_.rdbuf());
        old_cerr_ = std::cerr.rdbuf(buffer_.rdbuf());
    }
    ~OutputCapture() {
        std::cout.rdbuf(old_cout_);
        std::cerr.rdbuf(old_cerr_);
    }
    std::string str() const { return buffer_.str(); }
};

// 1. --help flag parsing
TEST_CASE(Phase26_01_HelpFlag) {
    const char* argv[] = {"bharatopt_cli", "--help"};
    CLIOptions opts = CLIParser::parse_args(2, const_cast<char**>(argv));
    EXPECT_TRUE(opts.help);
    EXPECT_TRUE(opts.valid);
}

// 2. Missing arguments (no args provided)
TEST_CASE(Phase26_02_MissingArguments) {
    const char* argv[] = {"bharatopt_cli"};
    CLIOptions opts = CLIParser::parse_args(1, const_cast<char**>(argv));
    EXPECT_TRUE(opts.help);
}

// 3. --file parsing
TEST_CASE(Phase26_03_FileParsing) {
    const char* argv[] = {"bharatopt_cli", "--file", "benchmarks/netlib/afiro.mps"};
    CLIOptions opts = CLIParser::parse_args(3, const_cast<char**>(argv));
    EXPECT_TRUE(opts.valid);
    EXPECT_EQ(opts.input_file, "benchmarks/netlib/afiro.mps");
}

// 4. Missing file value
TEST_CASE(Phase26_04_MissingFileValue) {
    const char* argv[] = {"bharatopt_cli", "--file"};
    CLIOptions opts = CLIParser::parse_args(2, const_cast<char**>(argv));
    EXPECT_FALSE(opts.valid);
    EXPECT_TRUE(opts.error_message.find("--file requires a file path") != std::string::npos);
}

// 5. Invalid file path handling
TEST_CASE(Phase26_05_InvalidFilePath) {
    const char* argv[] = {"bharatopt_cli", "--file", "nonexistent_file_path_xyz.mps"};
    CLIOptions opts = CLIParser::parse_args(3, const_cast<char**>(argv));
    EXPECT_TRUE(opts.valid); // Parser succeeds; run() returns exit code 1 safely
    int res = CLIDashboardApp::run(3, const_cast<char**>(argv));
    EXPECT_EQ(res, 1);
}

// 6. Valid solver: auto
TEST_CASE(Phase26_06_ValidSolverAuto) {
    const char* argv[] = {"bharatopt_cli", "--file", "benchmarks/netlib/afiro.mps", "--solver", "auto"};
    CLIOptions opts = CLIParser::parse_args(5, const_cast<char**>(argv));
    EXPECT_TRUE(opts.valid);
    EXPECT_EQ(static_cast<int>(opts.solver), static_cast<int>(CLISolverChoice::AUTO));
}

// 7. Valid solver: revised
TEST_CASE(Phase26_07_ValidSolverRevised) {
    const char* argv[] = {"bharatopt_cli", "--file", "benchmarks/netlib/afiro.mps", "--solver", "revised"};
    CLIOptions opts = CLIParser::parse_args(5, const_cast<char**>(argv));
    EXPECT_TRUE(opts.valid);
    EXPECT_EQ(static_cast<int>(opts.solver), static_cast<int>(CLISolverChoice::REVISED));
}

// 8. Valid solver: dual
TEST_CASE(Phase26_08_ValidSolverDual) {
    const char* argv[] = {"bharatopt_cli", "--file", "benchmarks/netlib/afiro.mps", "--solver", "dual"};
    CLIOptions opts = CLIParser::parse_args(5, const_cast<char**>(argv));
    EXPECT_TRUE(opts.valid);
    EXPECT_EQ(static_cast<int>(opts.solver), static_cast<int>(CLISolverChoice::DUAL));
}

// 9. Valid solver: pdhg
TEST_CASE(Phase26_09_ValidSolverPdhg) {
    const char* argv[] = {"bharatopt_cli", "--file", "benchmarks/netlib/afiro.mps", "--solver", "pdhg"};
    CLIOptions opts = CLIParser::parse_args(5, const_cast<char**>(argv));
    EXPECT_TRUE(opts.valid);
    EXPECT_EQ(static_cast<int>(opts.solver), static_cast<int>(CLISolverChoice::PDHG));
}

// 10. Valid solver: bnb
TEST_CASE(Phase26_10_ValidSolverBnb) {
    const char* argv[] = {"bharatopt_cli", "--file", "benchmarks/miplib/p0033.mps", "--solver", "bnb"};
    CLIOptions opts = CLIParser::parse_args(5, const_cast<char**>(argv));
    EXPECT_TRUE(opts.valid);
    EXPECT_EQ(static_cast<int>(opts.solver), static_cast<int>(CLISolverChoice::BNB));
}

// 11. Invalid solver choice
TEST_CASE(Phase26_11_InvalidSolverChoice) {
    const char* argv[] = {"bharatopt_cli", "--file", "benchmarks/netlib/afiro.mps", "--solver", "quantum"};
    CLIOptions opts = CLIParser::parse_args(5, const_cast<char**>(argv));
    EXPECT_FALSE(opts.valid);
    EXPECT_TRUE(opts.error_message.find("Invalid solver choice") != std::string::npos);
}

// 12. --presolve on
TEST_CASE(Phase26_12_PresolveOn) {
    const char* argv[] = {"bharatopt_cli", "--file", "benchmarks/netlib/afiro.mps", "--presolve", "on"};
    CLIOptions opts = CLIParser::parse_args(5, const_cast<char**>(argv));
    EXPECT_TRUE(opts.valid);
    EXPECT_TRUE(opts.presolve);
}

// 13. --presolve off
TEST_CASE(Phase26_13_PresolveOff) {
    const char* argv[] = {"bharatopt_cli", "--file", "benchmarks/netlib/afiro.mps", "--presolve", "off"};
    CLIOptions opts = CLIParser::parse_args(5, const_cast<char**>(argv));
    EXPECT_TRUE(opts.valid);
    EXPECT_FALSE(opts.presolve);
}

// 14. Invalid presolve value
TEST_CASE(Phase26_14_InvalidPresolveValue) {
    const char* argv[] = {"bharatopt_cli", "--file", "benchmarks/netlib/afiro.mps", "--presolve", "maybe"};
    CLIOptions opts = CLIParser::parse_args(5, const_cast<char**>(argv));
    EXPECT_FALSE(opts.valid);
    EXPECT_TRUE(opts.error_message.find("Invalid presolve choice") != std::string::npos);
}

// 15. --verify flag
TEST_CASE(Phase26_15_VerifyFlag) {
    const char* argv[] = {"bharatopt_cli", "--file", "benchmarks/netlib/afiro.mps", "--verify"};
    CLIOptions opts = CLIParser::parse_args(4, const_cast<char**>(argv));
    EXPECT_TRUE(opts.valid);
    EXPECT_TRUE(opts.verify);
}

// 16. --benchmark-summary flag
TEST_CASE(Phase26_16_BenchmarkSummaryFlag) {
    const char* argv[] = {"bharatopt_cli", "--benchmark-summary"};
    CLIOptions opts = CLIParser::parse_args(2, const_cast<char**>(argv));
    EXPECT_TRUE(opts.valid);
    EXPECT_TRUE(opts.benchmark_summary);
}

// 17. Combined CLI options
TEST_CASE(Phase26_17_CombinedOptions) {
    const char* argv[] = {"bharatopt_cli", "--file", "benchmarks/netlib/afiro.mps", "--solver", "dual", "--presolve", "on", "--verify"};
    CLIOptions opts = CLIParser::parse_args(8, const_cast<char**>(argv));
    EXPECT_TRUE(opts.valid);
    EXPECT_EQ(opts.input_file, "benchmarks/netlib/afiro.mps");
    EXPECT_EQ(static_cast<int>(opts.solver), static_cast<int>(CLISolverChoice::DUAL));
    EXPECT_TRUE(opts.presolve);
    EXPECT_TRUE(opts.verify);
}

// 18. Dashboard formatting rendering test
TEST_CASE(Phase26_18_DashboardFormatting) {
    OutputCapture capture;
    TerminalDashboard::render_header();
    std::string out = capture.str();
    EXPECT_TRUE(out.find("BHARATOPT OPTIMISATION SOLVER ENGINE") != std::string::npos);
    EXPECT_TRUE(out.find("SIH 2026 PS 26119") != std::string::npos);
}

// 19. Telemetry JSON ingestion
TEST_CASE(Phase26_19_TelemetryJsonIngestion) {
    std::vector<FinalBenchmarkRecord> records = FinalBenchmarkSuite::run_full_suite();
    bool exported = FinalBenchmarkReporter::export_json(records, "phase-25-final-benchmark.json");
    EXPECT_TRUE(exported);
    std::ifstream fs("phase-25-final-benchmark.json");
    EXPECT_TRUE(fs.is_open());
}

// 20. Telemetry CSV ingestion
TEST_CASE(Phase26_20_TelemetryCsvIngestion) {
    std::vector<FinalBenchmarkRecord> records = FinalBenchmarkSuite::run_full_suite();
    bool exported = FinalBenchmarkReporter::export_csv(records, "phase-25-final-benchmark.csv");
    EXPECT_TRUE(exported);
    std::ifstream fs("phase-25-final-benchmark.csv");
    EXPECT_TRUE(fs.is_open());
}

// 21. Missing telemetry handling
TEST_CASE(Phase26_21_MissingTelemetryHandling) {
    OutputCapture capture;
    TerminalDashboard::render_benchmark_summary("nonexistent_telemetry.json", "nonexistent_telemetry.csv");
    std::string out = capture.str();
    EXPECT_TRUE(out.find("Benchmark telemetry : NOT_AVAILABLE") != std::string::npos);
}

// 22. Invalid telemetry handling
TEST_CASE(Phase26_22_InvalidTelemetryHandling) {
    {
        std::ofstream fs("invalid_telemetry.json");
        fs << "{ malformed json content }";
    }
    OutputCapture capture;
    TerminalDashboard::render_benchmark_summary("invalid_telemetry.json", "invalid_telemetry.csv");
    std::string out = capture.str();
    EXPECT_TRUE(out.find("PHASE 25 FINAL BENCHMARK SUMMARY") != std::string::npos);
}

// 23. Native CUDA NOT_AVAILABLE display
TEST_CASE(Phase26_23_NativeCudaNotAvailableDisplay) {
    OutputCapture capture;
    TerminalDashboard::render_environment_section();
    std::string out = capture.str();
    EXPECT_TRUE(out.find("Native CUDA Status  : NOT_AVAILABLE") != std::string::npos);
}

// 24. HiGHS NOT_AVAILABLE display
TEST_CASE(Phase26_24_HighsNotAvailableDisplay) {
    OutputCapture capture;
    TerminalDashboard::render_environment_section();
    std::string out = capture.str();
    EXPECT_TRUE(out.find("HiGHS Oracle Status : NOT_AVAILABLE") != std::string::npos);
}

// 25. 1M CONSTRUCTED classification display
TEST_CASE(Phase26_25_1MConstructedDisplay) {
    OutputCapture capture;
    TerminalDashboard::render_benchmark_summary();
    std::string out = capture.str();
    EXPECT_TRUE(out.find("1M Scalability      : CONSTRUCTED") != std::string::npos);
}

// 26. Solver result display
TEST_CASE(Phase26_26_SolverResultDisplay) {
    OutputCapture capture;
    TerminalDashboard::render_solve_section("OPTIMAL", -464.753143, 10, 0, 1.23, 2.50);
    std::string out = capture.str();
    EXPECT_TRUE(out.find("Solver Status       : OPTIMAL") != std::string::npos);
    EXPECT_TRUE(out.find("-464.753143") != std::string::npos);
}

// 27. Verification PASS display
TEST_CASE(Phase26_27_VerificationPassDisplay) {
    VerificationResult v_res;
    v_res.verified = true;
    v_res.status_message = "All constraints and bounds satisfied";
    v_res.objective_recomputed = -464.753143;

    OutputCapture capture;
    TerminalDashboard::render_verification_section(true, &v_res);
    std::string out = capture.str();
    EXPECT_TRUE(out.find("SolutionVerifier    : PASS") != std::string::npos);
}

// 28. Verification FAIL display
TEST_CASE(Phase26_28_VerificationFailDisplay) {
    VerificationResult v_res;
    v_res.verified = false;
    v_res.status_message = "Constraint violation detected";
    v_res.maximum_constraint_violation = 1e-2;

    OutputCapture capture;
    TerminalDashboard::render_verification_section(true, &v_res);
    std::string out = capture.str();
    EXPECT_TRUE(out.find("SolutionVerifier    : FAIL") != std::string::npos);
}

// 29. Invalid CLI input does not crash
TEST_CASE(Phase26_29_InvalidCliInputNoCrash) {
    const char* argv[] = {"bharatopt_cli", "--unknown-flag", "value", "--file"};
    OutputCapture capture;
    int res = CLIDashboardApp::run(4, const_cast<char**>(argv));
    EXPECT_EQ(res, 1);
    std::string out = capture.str();
    EXPECT_TRUE(out.find("Unknown command line option") != std::string::npos);
}

// 30. Help output contains documented syntax
TEST_CASE(Phase26_30_HelpOutputContainsDocumentedSyntax) {
    OutputCapture capture;
    CLIParser::print_help();
    std::string out = capture.str();
    EXPECT_TRUE(out.find("bharatopt_cli --file <path.mps>") != std::string::npos);
    EXPECT_TRUE(out.find("--solver, -s <choice>") != std::string::npos);
    EXPECT_TRUE(out.find("--presolve, -p <on|off>") != std::string::npos);
    EXPECT_TRUE(out.find("--verify, -v") != std::string::npos);
    EXPECT_TRUE(out.find("--benchmark-summary, -b") != std::string::npos);
}

} // namespace bharatopt
