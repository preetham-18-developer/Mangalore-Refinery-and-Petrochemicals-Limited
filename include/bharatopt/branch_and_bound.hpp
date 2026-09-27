#ifndef BHARATOPT_BRANCH_AND_BOUND_HPP
#define BHARATOPT_BRANCH_AND_BOUND_HPP

#include <string>
#include <vector>
#include <memory>
#include <queue>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>
#include <bharatopt/milp_foundation.hpp>

namespace bharatopt {

enum class BnBNodeStatus {
    OPEN,
    PROCESSING,
    INTEGER_FEASIBLE,
    PRUNED_INFEASIBLE,
    PRUNED_BOUND,
    BRANCHED,
    NUMERICAL_FAILURE,
    UNRESOLVED
};

std::string bnb_node_status_to_string(BnBNodeStatus status);

enum class BnBSolverStatus {
    OPTIMAL,
    INFEASIBLE,
    LIMIT_REACHED,
    NUMERICAL_FAILURE,
    UNRESOLVED,
    UNSUPPORTED_MODEL
};

std::string bnb_solver_status_to_string(BnBSolverStatus status);

enum class WarmStartMode {
    COLD_START,
    WARM_START
};

enum class WarmStartStatus {
    WARM_START_USED,
    WARM_START_REJECTED,
    WARM_START_FAILED,
    COLD_START_USED
};

struct LPWarmStartState {
    bool valid{false};
    size_t num_rows{0};
    size_t num_cols{0};
    Basis basis;
    std::vector<real_t> primal_solution;
    std::vector<real_t> dual_solution;
    real_t lp_obj{0.0};
};

struct VariableBoundChange {
    index_t var_index{-1};
    real_t new_lower_bound{0.0};
    real_t new_upper_bound{0.0};
};

struct BnBNode {
    index_t node_id{-1};
    index_t parent_node_id{-1};
    int depth{0};

    std::vector<VariableBoundChange> bound_changes;
    std::vector<real_t> node_lower_bounds;
    std::vector<real_t> node_upper_bounds;

    real_t lp_obj_bound{0.0};
    std::vector<real_t> lp_solution;
    bool is_lp_optimal{false};
    bool is_lp_infeasible{false};

    index_t branched_var_idx{-1};
    real_t branched_val{0.0};

    BnBNodeStatus status{BnBNodeStatus::OPEN};
    LPWarmStartState warm_start_state;
};

struct BnBConfig {
    size_t max_nodes{10000};
    double time_limit_ms{60000.0};
    real_t integrality_tolerance{1e-5};
    real_t objective_tolerance{1e-7};
    real_t feasibility_tolerance{1e-4};
    WarmStartMode warm_start_mode{WarmStartMode::WARM_START};
};

struct Incumbent {
    bool has_incumbent{false};
    real_t objective_value{0.0};
    std::vector<real_t> solution;
    index_t originating_node_id{-1};
    IntegerVerificationResult verification;
};

struct BnBTelemetry {
    double total_solve_time_ms{0.0};
    double root_relaxation_time_ms{0.0};
    size_t nodes_created{0};
    size_t nodes_processed{0};
    size_t nodes_pruned_infeasibility{0};
    size_t nodes_pruned_bound{0};
    size_t integer_feasible_nodes{0};
    size_t fractional_nodes{0};
    int max_tree_depth{0};
    size_t incumbent_updates{0};
    real_t final_incumbent_obj{0.0};
    real_t best_open_bound{0.0};
    BnBSolverStatus status{BnBSolverStatus::UNRESOLVED};

    // Phase 19 Warm Start Telemetry
    WarmStartMode warm_start_mode{WarmStartMode::WARM_START};
    size_t warm_starts_attempted{0};
    size_t warm_starts_accepted{0};
    size_t warm_starts_rejected{0};
    size_t warm_starts_failed{0};
    size_t cold_fallbacks{0};

    std::string to_string() const;
};

struct BnBResult {
    BnBSolverStatus status{BnBSolverStatus::UNRESOLVED};
    real_t objective_value{0.0};
    std::vector<real_t> solution;
    Incumbent incumbent;
    BnBTelemetry telemetry;
    std::string status_message;
};

class BranchAndBoundEngine {
public:
    explicit BranchAndBoundEngine(BnBConfig config = BnBConfig());

    // Main solver entry point
    BnBResult solve(const LPModel& model);

    // Configuration accessors
    const BnBConfig& config() const { return config_; }
    void set_config(const BnBConfig& config) { config_ = config; }

private:
    BnBConfig config_;

    // Helper: Select variable with largest fractional part
    index_t select_branching_variable(
        const LPModel& model,
        const std::vector<real_t>& solution,
        real_t& out_val
    ) const;

    // Helper: Compare candidate objective against incumbent
    bool is_better_than_incumbent(
        real_t candidate_obj,
        const Incumbent& incumbent,
        ObjectiveSense sense
    ) const;

    // Helper: Check if node bound cannot improve incumbent
    bool can_prune_by_bound(
        real_t node_bound,
        const Incumbent& incumbent,
        ObjectiveSense sense
    ) const;
};

} // namespace bharatopt

#endif // BHARATOPT_BRANCH_AND_BOUND_HPP
