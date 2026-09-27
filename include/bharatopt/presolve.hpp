#ifndef BHARATOPT_PRESOLVE_HPP
#define BHARATOPT_PRESOLVE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>

namespace bharatopt {

enum class PresolveStatus {
    SUCCESS,
    INFEASIBLE,
    UNBOUNDED,
    NO_CHANGE
};

enum class ReductionType {
    FIXED_VARIABLE,
    EMPTY_ROW_REDUNDANT,
    EMPTY_ROW_INFEASIBLE,
    EMPTY_COL_FIXED,
    EMPTY_COL_UNBOUNDED,
    SINGLETON_ROW,
    BOUND_TIGHTENING,
    REDUNDANT_CONSTRAINT
};

struct TransformationRecord {
    ReductionType type;
    std::string description;
    std::string entity_name;
    index_t orig_var_idx{-1};
    index_t orig_cons_idx{-1};
    real_t value{0.0};
};

class Postsolve {
public:
    Postsolve() = default;

    void initialize(const LPModel& orig_model);
    void record_fixed_var(index_t orig_var_idx, real_t val);
    void record_var_map(index_t orig_var_idx, index_t reduced_var_idx);

    // Solution mapping: reduced space -> original space
    std::vector<real_t> recover_solution(const std::vector<real_t>& reduced_x) const;

    // Verification against original model
    bool verify_original_feasibility(const std::vector<real_t>& orig_x, real_t tol = DEFAULT_FEASIBILITY_TOLERANCE) const;
    real_t compute_original_objective(const std::vector<real_t>& orig_x) const;

    const LPModel& original_model() const { return orig_model_copy_; }

private:
    size_t orig_num_vars_{0};
    LPModel orig_model_copy_;
    std::vector<real_t> fixed_values_;
    std::vector<bool> is_fixed_;
    std::vector<index_t> orig_to_reduced_var_;
};

struct PresolveStatistics {
    size_t original_vars{0};
    size_t original_cons{0};
    size_t reduced_vars{0};
    size_t reduced_cons{0};
    size_t vars_removed{0};
    size_t cons_removed{0};
    double presolve_time_ms{0.0};
    std::vector<TransformationRecord> transformations;
};

struct PresolveResult {
    PresolveStatus status{PresolveStatus::NO_CHANGE};
    LPModel reduced_model;
    Postsolve postsolve;
    PresolveStatistics stats;

    std::string to_string() const;
};

class PresolveEngine {
public:
    PresolveEngine() = default;

    // Presolve entry point
    PresolveResult presolve(const LPModel& original_model) const;
};

} // namespace bharatopt

#endif // BHARATOPT_PRESOLVE_HPP
