#ifndef BHARATOPT_MILP_FOUNDATION_HPP
#define BHARATOPT_MILP_FOUNDATION_HPP

#include <string>
#include <vector>
#include <memory>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>
#include <bharatopt/model_validator.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/dual_revised_simplex.hpp>

namespace bharatopt {

enum class ModelType {
    LP,
    MILP,
    UNSUPPORTED_MODEL
};

std::string model_type_to_string(ModelType type);

struct RelaxationMapping {
    std::vector<index_t> milp_to_lp;
    std::vector<index_t> lp_to_milp;
    std::vector<VariableType> original_types;
    std::vector<std::string> var_names;
};

struct LPRelaxation {
    LPModel relaxation_model;
    RelaxationMapping mapping;
};

struct IntegerVerificationResult {
    bool is_integer_feasible{false};
    real_t max_integrality_violation{0.0};
    size_t violating_variable_count{0};
    real_t max_bound_violation{0.0};
    real_t max_constraint_residual{0.0};
    bool constraint_feasibility_passed{false};
    bool bounds_passed{false};
    std::vector<index_t> violating_var_indices;
    std::string diagnostics;

    std::string to_string() const;
};

enum class MilpSolutionStatus {
    LP_RELAXATION_OPTIMAL,
    INTEGER_FEASIBLE_SOLUTION,
    INTEGER_INFEASIBLE_SOLUTION,
    NUMERICAL_FAILURE,
    UNSUPPORTED_MODEL
};

std::string milp_solution_status_to_string(MilpSolutionStatus status);

struct MilpRelaxationResult {
    MilpSolutionStatus status{MilpSolutionStatus::NUMERICAL_FAILURE};
    ModelType model_type{ModelType::UNSUPPORTED_MODEL};
    LPModel relaxation_model;
    RelaxationMapping mapping;
    real_t objective_value{0.0};
    std::vector<real_t> primal_solution;
    IntegerVerificationResult integer_verification;

    double classification_time_ms{0.0};
    double relaxation_time_ms{0.0};
    double solve_time_ms{0.0};
    double integer_check_time_ms{0.0};
    double total_time_ms{0.0};

    std::string solver_used;
    std::string status_message;
};

class MilpFoundation {
public:
    // 1. Classification
    static ModelType classify_model(const LPModel& model);

    // 2. LP Relaxation Extraction
    static LPRelaxation extract_relaxation(const LPModel& model);

    // 3. Integer Feasibility Verification
    static IntegerVerificationResult verify_integer_feasibility(
        const LPModel& original_milp,
        const std::vector<real_t>& solution,
        real_t integrality_tolerance = 1e-5
    );

    // 4. End-to-End LP Relaxation Solver
    static MilpRelaxationResult solve_relaxation(
        const LPModel& model,
        real_t integrality_tolerance = 1e-5
    );
};

} // namespace bharatopt

#endif // BHARATOPT_MILP_FOUNDATION_HPP
