#ifndef BHARATOPT_ABLATION_STUDY_HPP
#define BHARATOPT_ABLATION_STUDY_HPP

#include <string>
#include <vector>
#include <memory>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>
#include <bharatopt/presolve.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/dual_revised_simplex.hpp>
#include <bharatopt/branch_and_bound.hpp>
#include <bharatopt/solution_verifier.hpp>
#include <bharatopt/execution_router.hpp>
#include <bharatopt/benchmark_framework.hpp>

namespace bharatopt {

enum class AblationExperimentType {
    A1_PRESOLVE,
    A2_SPARSE_REPRESENTATION,
    A3_ADAPTIVE_ROUTING,
    A4_WARM_START,
    A5_VERIFICATION_OVERHEAD,
    A6_COST_ESTIMATOR,
    A7_BASIS_PROPAGATION
};

std::string ablation_experiment_type_to_string(AblationExperimentType type);

struct AblationRecord {
    std::string experiment_id;
    std::string component;
    std::string baseline_config;
    std::string ablated_config;
    std::string instance_name;
    std::string problem_type; // "LP" or "MILP"
    std::string solver_name;
    std::string hardware{"CPU"};
    std::string build_type{"Release"};
    size_t repetitions{5};

    // Baseline measured metrics
    double baseline_solve_time_ms{0.0};
    double baseline_total_time_ms{0.0};
    size_t baseline_iterations{0};
    size_t baseline_node_count{0};
    real_t baseline_objective{0.0};
    std::string baseline_status;

    // Ablated measured metrics
    double ablated_solve_time_ms{0.0};
    double ablated_total_time_ms{0.0};
    size_t ablated_iterations{0};
    size_t ablated_node_count{0};
    real_t ablated_objective{0.0};
    std::string ablated_status;

    // Comparative calculations
    double delta_solve_time_ms{0.0};
    double percent_change_solve_time{0.0};
    double delta_total_time_ms{0.0};
    double percent_change_total_time{0.0};

    // Correctness verification
    bool baseline_verified{false};
    bool ablated_verified{false};
    real_t max_residual{0.0};

    std::string experiment_status{"PASS"}; // "PASS", "NOT_AVAILABLE", "UNSUPPORTED", "FAIL"
    std::string notes;
};

class AblationHarness {
public:
    static AblationRecord run_a1_presolve(const LPModel& model, const std::string& instance_name, size_t repetitions = 5);
    static AblationRecord run_a2_sparse(const LPModel& model, const std::string& instance_name, size_t repetitions = 5);
    static AblationRecord run_a3_adaptive_routing(const LPModel& model, const std::string& instance_name, size_t repetitions = 5);
    static AblationRecord run_a4_warm_start(const LPModel& model, const std::string& instance_name, size_t repetitions = 5);
    static AblationRecord run_a5_verification_overhead(const LPModel& model, const std::string& instance_name, size_t repetitions = 5);
    static AblationRecord run_a6_cost_estimator(const LPModel& model, const std::string& instance_name, size_t repetitions = 5);
    static AblationRecord run_a7_basis_propagation(const LPModel& model, const std::string& instance_name, size_t repetitions = 5);

    static std::vector<AblationRecord> run_all_ablations(
        const std::vector<std::pair<std::string, LPModel>>& lp_instances,
        const std::vector<std::pair<std::string, LPModel>>& milp_instances,
        size_t repetitions = 5
    );
};

class AblationReporter {
public:
    static bool export_csv(const std::vector<AblationRecord>& records, const std::string& filepath);
    static bool export_json(const std::vector<AblationRecord>& records, const std::string& filepath);
    static void print_summary_table(const std::vector<AblationRecord>& records);
};

} // namespace bharatopt

#endif // BHARATOPT_ABLATION_STUDY_HPP
