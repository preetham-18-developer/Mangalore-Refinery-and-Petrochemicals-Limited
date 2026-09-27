#ifndef BHARATOPT_REVISED_SIMPLEX_HPP
#define BHARATOPT_REVISED_SIMPLEX_HPP

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>
#include <bharatopt/educational_simplex.hpp> // For EnteringRule

namespace bharatopt {

enum class VariableStatus : uint8_t {
    BASIC,
    NONBASIC_LOWER,
    NONBASIC_UPPER,
    FREE
};

enum class RevisedSimplexStatus {
    OPTIMAL,
    UNBOUNDED,
    INFEASIBLE,
    UNSUPPORTED_INITIAL_BASIS,
    ITERATION_LIMIT,
    NUMERICAL_FAILURE
};

enum class BasisSolverType {
    DENSE_LU,  // Phase 7 Dense Reference Solver
    SPARSE_LU  // Phase 8 Sparse LU Factorization Solver
};

struct RevisedSimplexOptions {
    size_t max_iterations{10000};
    real_t pivot_tolerance{DEFAULT_PIVOT_TOLERANCE};
    real_t optimality_tolerance{DEFAULT_OPTIMALITY_TOLERANCE};
    real_t feasibility_tolerance{DEFAULT_FEASIBILITY_TOLERANCE};
    real_t zero_tolerance{DEFAULT_ZERO_TOLERANCE};
    EnteringRule entering_rule{EnteringRule::BLANDS_RULE};
    BasisSolverType solver_type{BasisSolverType::DENSE_LU};
    bool enable_incremental_updates{true};
    size_t max_eta_updates{50};
    bool enable_trace{false};
    bool verbose_output{false};
};

/**
 * Basis Management Class.
 * Tracks basic/non-basic variables, basis positions, and variable statuses.
 * Enforces basis invariants:
 * 1. num_basic == num_rows
 * 2. num_nonbasic == num_cols - num_rows
 * 3. Partition is disjoint and complete
 * 4. Index mappings var_to_basic_pos and var_to_nonbasic_pos are consistent
 */
class Basis {
public:
    size_t num_rows{0}; // m (number of basic variables)
    size_t num_cols{0}; // n (total variables in standard form)

    std::vector<index_t> basic_vars;        // Variable indices in basis (size m)
    std::vector<index_t> nonbasic_vars;     // Variable indices not in basis (size n - m)
    std::vector<index_t> var_to_basic_pos;  // Maps var_idx -> pos in basic_vars (-1 if non-basic)
    std::vector<index_t> var_to_nonbasic_pos; // Maps var_idx -> pos in nonbasic_vars (-1 if basic)
    std::vector<VariableStatus> var_status; // Variable status flags

    Basis() = default;
    Basis(size_t m, size_t n);

    // Initialize basis from explicit basic variable indices
    bool set_initial_basis(const std::vector<index_t>& initial_basic_vars);

    // Update basis: replace basic variable at basic_pos with entering_var
    bool update_basis(size_t basic_pos, index_t entering_var);

    // Check mathematical and structural invariants
    bool check_invariants() const;
};

/**
 * Standard Form LP representation for Revised Simplex computational kernel.
 * Minimization form:
 *     min c^T x + c0
 *     s.t. A x = b, x >= 0
 */
struct StandardFormLP {
    size_t num_rows{0}; // m
    size_t num_cols{0}; // n (original decision + slacks + surplus + artificials)
    size_t orig_vars{0};
    
    // Matrix A stored in dense 2D for Phase 7 reference basis solve (m x n)
    std::vector<std::vector<real_t>> A;
    std::vector<real_t> b;           // RHS vector (size m, b >= 0)
    std::vector<real_t> c;           // Objective coefficient vector (size n)
    real_t c0{0.0};                  // Constant objective offset
    ObjectiveSense orig_sense{ObjectiveSense::MINIMIZE};

    std::vector<std::string> col_names;
    std::vector<index_t> col_to_orig_var;
    std::vector<real_t> var_shifts;
    std::vector<bool> is_artificial;
    std::vector<bool> is_slack;
    std::vector<bool> is_surplus;
};

/**
 * Modular Basis Solver interface.
 * Decouples Revised Simplex from specific basis matrix factorizations.
 * Phase 7 provides DenseBasisSolver; Phase 8 will introduce SparseLUBasisSolver.
 */
class IBasisSolver {
public:
    virtual ~IBasisSolver() = default;
    virtual bool factorize(const StandardFormLP& lp, const Basis& basis) = 0;
    virtual bool solve_primal(const std::vector<real_t>& rhs, std::vector<real_t>& x) const = 0;
    virtual bool solve_dual(const std::vector<real_t>& rhs, std::vector<real_t>& y) const = 0;
};

/**
 * Reference Dense LU Basis Solver using Gaussian Elimination with partial pivoting.
 */
class DenseBasisSolver : public IBasisSolver {
public:
    bool factorize(const StandardFormLP& lp, const Basis& basis) override;
    bool solve_primal(const std::vector<real_t>& rhs, std::vector<real_t>& x) const override;
    bool solve_dual(const std::vector<real_t>& rhs, std::vector<real_t>& y) const override;

private:
    size_t m_{0};
    std::vector<std::vector<real_t>> LU_;
    std::vector<size_t> pivot_perm_;
};

struct RevisedSimplexResult {
    RevisedSimplexStatus status{RevisedSimplexStatus::NUMERICAL_FAILURE};
    std::vector<real_t> primal_solution; // Original space decision variables x in R^n
    std::vector<real_t> dual_solution;   // Dual variables y in R^m
    std::vector<real_t> reduced_costs;   // Reduced costs r in R^n
    real_t objective_value{0.0};         // Recomputed original model objective
    size_t iterations{0};
    Basis final_basis;
    std::string message;
};

class RevisedSimplex {
public:
    explicit RevisedSimplex(RevisedSimplexOptions options = {});

    // Main solver entry point for LPModel
    RevisedSimplexResult solve(const LPModel& model);

    // Convert LPModel into standard form minimization LP
    StandardFormLP create_standard_form(const LPModel& model) const;

    // Helper functions exposed for independent unit testing
    bool compute_basic_solution(IBasisSolver& solver,
                                const StandardFormLP& lp,
                                const Basis& basis,
                                std::vector<real_t>& x_B) const;

    bool compute_dual_vector(IBasisSolver& solver,
                             const StandardFormLP& lp,
                             const Basis& basis,
                             std::vector<real_t>& y) const;

    void compute_reduced_costs(const StandardFormLP& lp,
                               const std::vector<real_t>& y,
                               std::vector<real_t>& r) const;

    index_t select_entering_variable(const Basis& basis,
                                     const std::vector<real_t>& r,
                                     const StandardFormLP& lp) const;

    bool compute_direction(IBasisSolver& solver,
                           const StandardFormLP& lp,
                           index_t entering_var,
                           std::vector<real_t>& d_B) const;

    index_t ratio_test(const std::vector<real_t>& x_B,
                       const std::vector<real_t>& d_B,
                       const Basis& basis,
                       real_t& min_ratio) const;

    // Independent feasibility verification
    bool verify_solution_feasibility(const LPModel& model,
                                     const std::vector<real_t>& solution,
                                     real_t tol = DEFAULT_FEASIBILITY_TOLERANCE) const;

    // Independent objective recomputation
    real_t recompute_original_objective(const LPModel& model,
                                        const std::vector<real_t>& solution) const;

    const RevisedSimplexOptions& options() const { return options_; }
    void set_options(const RevisedSimplexOptions& options) { options_ = options; }

private:
    RevisedSimplexOptions options_;
};

} // namespace bharatopt

#endif // BHARATOPT_REVISED_SIMPLEX_HPP
