#ifndef BHARATOPT_EDUCATIONAL_SIMPLEX_HPP
#define BHARATOPT_EDUCATIONAL_SIMPLEX_HPP

#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>

namespace bharatopt {

enum class SimplexStatus {
    OPTIMAL,
    UNBOUNDED,
    INFEASIBLE,
    UNSUPPORTED_INITIAL_BASIS,
    ITERATION_LIMIT,
    NUMERICAL_FAILURE
};

enum class EnteringRule {
    MOST_NEGATIVE, // Dantzig's rule: most negative reduced cost for maximization
    BLANDS_RULE   // Anti-cycling rule: smallest variable index with negative reduced cost
};

struct SimplexOptions {
    size_t max_iterations{1000};
    real_t pivot_tolerance{DEFAULT_PIVOT_TOLERANCE};
    real_t optimality_tolerance{DEFAULT_OPTIMALITY_TOLERANCE};
    real_t zero_tolerance{DEFAULT_ZERO_TOLERANCE};
    EnteringRule entering_rule{EnteringRule::BLANDS_RULE};
    bool enable_trace{false};
    bool verbose_output{false};
};

/**
 * Explicit Educational Simplex Tableau representation.
 * 
 * Matrix layout ((num_rows + 1) x (num_cols + 1)):
 * - Rows 0 ... m-1: Constraint equations
 * - Row m: Objective / Reduced cost row (z - sum(c_j * x_j) = c0)
 * - Col 0 ... n-1: Variable coefficients (original, slacks, surplus, artificials)
 * - Col n: Right-Hand Side (RHS) values b
 * - matrix[m][n]: -z (negative of current objective value including offset)
 * 
 * Sign convention for Maximization:
 * - Reduced cost for column j in row m is: T[m][j] = -c_j + z_j
 * - An entering variable has negative reduced cost (T[m][j] < -optimality_tolerance).
 */
struct Tableau {
    size_t num_rows{0}; // Number of constraint rows m
    size_t num_cols{0}; // Number of total variable columns n
    std::vector<std::vector<real_t>> matrix; // (m+1) x (n+1)
    std::vector<index_t> basis;              // Basic variable index for each row i
    std::vector<std::string> col_names;       // Column names
    std::vector<bool> is_slack;              // True if column is a slack variable
    std::vector<bool> is_surplus;            // True if column is a surplus variable
    std::vector<bool> is_artificial;         // True if column is an artificial variable
    std::vector<index_t> orig_var_map;       // Map from column index to original variable index (-1 if slack/art)
    std::vector<real_t> var_shifts;          // Shift l_j applied to original variable (x_j = x_j_tilde + l_j)
    real_t original_obj_offset{0.0};
    ObjectiveSense original_sense{ObjectiveSense::MAXIMIZE};

    void print(std::ostream& os = std::cout) const;
};

struct SimplexIterationTrace {
    size_t iteration{0};
    index_t entering_var{-1};
    index_t leaving_var{-1};
    size_t pivot_row{0};
    size_t pivot_col{0};
    real_t pivot_element{0.0};
    real_t objective_value{0.0};
    std::string details;
};

struct SimplexResult {
    SimplexStatus status{SimplexStatus::NUMERICAL_FAILURE};
    std::vector<real_t> primal_solution; // Solution vector in original variable space x in R^n
    real_t objective_value{0.0};         // Recomputed original model objective value c^T x + c0
    size_t iterations{0};
    std::vector<SimplexIterationTrace> trace;
    Tableau final_tableau;
    std::string message;
};

class EducationalSimplex {
public:
    explicit EducationalSimplex(SimplexOptions options = {});

    // Solve the given LPModel using explicit Tableau Simplex
    SimplexResult solve(const LPModel& model);

    // Create an initial standard form Tableau from an LPModel
    Tableau create_initial_tableau(const LPModel& model);

    // Step-by-step pivot operation on a tableau
    bool pivot(Tableau& tableau, size_t pivot_row, size_t pivot_col);

    // Select entering variable column index using configured entering rule (-1 if optimal)
    index_t select_entering_variable(const Tableau& tableau) const;

    // Select leaving variable row index using Minimum Ratio Test (-1 if unbounded)
    index_t select_leaving_variable(const Tableau& tableau, size_t entering_col) const;

    // Independent solution feasibility verification
    bool verify_solution_feasibility(const LPModel& model,
                                     const std::vector<real_t>& solution,
                                     real_t tol = DEFAULT_FEASIBILITY_TOLERANCE) const;

    // Independent original objective recomputation
    real_t recompute_original_objective(const LPModel& model,
                                        const std::vector<real_t>& solution) const;

    const SimplexOptions& options() const { return options_; }
    void set_options(const SimplexOptions& options) { options_ = options; }

private:
    SimplexOptions options_;

    // Canonicalize objective row by subtracting c_basic * basic_row
    void canonicalize_objective(Tableau& tableau);
};

} // namespace bharatopt

#endif // BHARATOPT_EDUCATIONAL_SIMPLEX_HPP
