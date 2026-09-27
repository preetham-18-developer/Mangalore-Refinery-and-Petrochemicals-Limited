#ifndef BHARATOPT_CLI_DASHBOARD_HPP
#define BHARATOPT_CLI_DASHBOARD_HPP

#include <string>
#include <vector>
#include <memory>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>
#include <bharatopt/mps_parser.hpp>
#include <bharatopt/presolve.hpp>
#include <bharatopt/execution_router.hpp>
#include <bharatopt/solution_verifier.hpp>
#include <bharatopt/final_benchmark.hpp>

namespace bharatopt {

enum class CLISolverChoice {
    AUTO,
    REVISED,
    DUAL,
    PDHG,
    BNB
};

struct CLIOptions {
    std::string input_file;
    CLISolverChoice solver{CLISolverChoice::AUTO};
    bool presolve{true};
    bool verify{false};
    bool benchmark_summary{false};
    bool help{false};

    bool valid{true};
    std::string error_message;
};

class CLIParser {
public:
    static CLIOptions parse_args(int argc, char* argv[]);
    static void print_help();
};

class TerminalDashboard {
public:
    static void render_header();
    static void render_model_section(const std::string& filepath, const MpsParseResult& parse_res, const LPModel& model);
    static void render_validation_section(const ValidationResult& val_res);
    static void render_presolve_section(bool enabled, const PresolveResult* presolve_res, double presolve_time_ms);
    static void render_router_section(const RoutingDecision& decision, bool is_auto);
    static void render_solve_section(const std::string& status, real_t objective, size_t iterations, size_t nodes, double solve_time_ms, double total_time_ms);
    static void render_infeasibility_section(const InfeasibilityDiagnosis& diag);
    static void render_verification_section(bool enabled, const VerificationResult* v_res);
    static void render_environment_section();
    static void render_benchmark_summary(const std::string& json_path = "phase-25-final-benchmark.json", const std::string& csv_path = "phase-25-final-benchmark.csv");
};

class CLIDashboardApp {
public:
    static int run(int argc, char* argv[]);
};

} // namespace bharatopt

#endif // BHARATOPT_CLI_DASHBOARD_HPP
