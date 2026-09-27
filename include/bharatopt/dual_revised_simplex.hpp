#ifndef BHARATOPT_DUAL_REVISED_SIMPLEX_HPP
#define BHARATOPT_DUAL_REVISED_SIMPLEX_HPP

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/sparse_lu.hpp>

namespace bharatopt {

enum class DualRevisedSimplexStatus {
    OPTIMAL,
    UNBOUNDED,
    INFEASIBLE,
    UNSUPPORTED_INITIAL_BASIS,
    ITERATION_LIMIT,
    NUMERICAL_FAILURE
};

struct DualRevisedSimplexOptions {
    size_t max_iterations{10000};
    real_t pivot_tolerance{DEFAULT_PIVOT_TOLERANCE};
    real_t optimality_tolerance{DEFAULT_OPTIMALITY_TOLERANCE};
    real_t feasibility_tolerance{DEFAULT_FEASIBILITY_TOLERANCE};
    real_t zero_tolerance{DEFAULT_ZERO_TOLERANCE};
    BasisSolverType solver_type{BasisSolverType::SPARSE_LU};
    bool enable_incremental_updates{true};
    size_t max_eta_updates{50};
    bool enable_trace{false};
    bool verbose_output{false};
};

struct DualRevisedSimplexResult {
    DualRevisedSimplexStatus status{DualRevisedSimplexStatus::NUMERICAL_FAILURE};
    std::vector<real_t> primal_solution; // Original space decision variables x in R^n
    std::vector<real_t> dual_solution;   // Dual variables y in R^m
    std::vector<real_t> reduced_costs;   // Reduced costs r in R^n
    real_t objective_value{0.0};         // Recomputed original model objective
    size_t iterations{0};
    Basis final_basis;
    std::string message;
};

/**
 * Dual Revised Simplex Engine.
 * Solves linear programs starting from a dual-feasible basis while driving primal infeasibilities to zero.
 * 
 * Mathematical Workflow:
 * 1. Start with dual-feasible basis (r_N >= -tol_opt).
 * 2. Compute primal basic solution x_B = B^(-1) b.
 * 3. Check primal feasibility: if x_B >= -tol_feas for all basic variables, status = OPTIMAL.
 * 4. Select leaving basic variable p with (x_B)_p < -tol_feas (Most Infeasible Rule).
 * 5. Compute row p of B^(-1) A via dual solve B^T w_p = e_p -> alpha_pj = w_p^T a_j.
 * 6. Dual Ratio Test for entering variable q: min_{j in N, alpha_pj < -tol_zero} (r_j / -alpha_pj).
 * 7. If no eligible entering variable exists (alpha_pj >= -tol_zero for all j), status = INFEASIBLE.
 * 8. Pivot basis: replace basic variable at pos p with entering variable q and refactorize B.
 */
class DualRevisedSimplex {
public:
    explicit DualRevisedSimplex(DualRevisedSimplexOptions options = {});

    // Main solver entry point for LPModel
    DualRevisedSimplexResult solve(const LPModel& model);

    // Warm-start solver entry point using pre-existing dual-feasible basis
    DualRevisedSimplexResult solve_warm_start(const LPModel& model, const Basis& initial_basis);

    // Convert LPModel into standard form minimization LP (sharing RevisedSimplex logic)
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

    bool check_dual_feasibility(const Basis& basis,
                                const std::vector<real_t>& r) const;

    index_t select_leaving_variable(const std::vector<real_t>& x_B,
                                    const Basis& basis,
                                    real_t& max_violation) const;

    bool compute_tableau_row(IBasisSolver& solver,
                             const StandardFormLP& lp,
                             size_t leaving_pos,
                             std::vector<real_t>& alpha_p) const;

    index_t dual_ratio_test(const Basis& basis,
                            const std::vector<real_t>& r,
                            const std::vector<real_t>& alpha_p,
                            real_t& min_ratio) const;

    // Independent feasibility verification
    bool verify_solution_feasibility(const LPModel& model,
                                     const std::vector<real_t>& solution,
                                     real_t tol = DEFAULT_FEASIBILITY_TOLERANCE) const;

    // Independent objective recomputation
    real_t recompute_original_objective(const LPModel& model,
                                        const std::vector<real_t>& solution) const;

    const DualRevisedSimplexOptions& options() const { return options_; }
    void set_options(const DualRevisedSimplexOptions& options) { options_ = options; }

private:
    DualRevisedSimplexOptions options_;
};

} // namespace bharatopt

#endif // BHARATOPT_DUAL_REVISED_SIMPLEX_HPP
