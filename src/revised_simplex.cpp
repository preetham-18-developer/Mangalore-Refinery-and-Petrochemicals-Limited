#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/sparse_lu.hpp>
#include <bharatopt/basis_update.hpp>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <iostream>

namespace bharatopt {

// ============================================================================
// BASIS CLASS IMPLEMENTATION
// ============================================================================

Basis::Basis(size_t m, size_t n)
    : num_rows(m), num_cols(n) {
    var_to_basic_pos.assign(n, -1);
    var_to_nonbasic_pos.assign(n, -1);
    var_status.assign(n, VariableStatus::NONBASIC_LOWER);
}

bool Basis::set_initial_basis(const std::vector<index_t>& initial_basic_vars) {
    if (initial_basic_vars.size() != num_rows) {
        return false;
    }

    basic_vars = initial_basic_vars;
    var_to_basic_pos.assign(num_cols, -1);
    var_to_nonbasic_pos.assign(num_cols, -1);
    var_status.assign(num_cols, VariableStatus::NONBASIC_LOWER);
    nonbasic_vars.clear();

    std::vector<bool> in_basis(num_cols, false);
    for (size_t pos = 0; pos < num_rows; ++pos) {
        index_t var = initial_basic_vars[pos];
        if (var < 0 || static_cast<size_t>(var) >= num_cols || in_basis[var]) {
            return false; // Invalid or duplicate basic variable
        }
        in_basis[var] = true;
        var_to_basic_pos[var] = static_cast<index_t>(pos);
        var_status[var] = VariableStatus::BASIC;
    }

    for (size_t var = 0; var < num_cols; ++var) {
        if (!in_basis[var]) {
            size_t nb_pos = nonbasic_vars.size();
            nonbasic_vars.push_back(static_cast<index_t>(var));
            var_to_nonbasic_pos[var] = static_cast<index_t>(nb_pos);
        }
    }

    return check_invariants();
}

bool Basis::update_basis(size_t basic_pos, index_t entering_var) {
    if (basic_pos >= num_rows || entering_var < 0 || static_cast<size_t>(entering_var) >= num_cols) {
        return false;
    }

    index_t leaving_var = basic_vars[basic_pos];
    index_t entering_nb_pos = var_to_nonbasic_pos[entering_var];

    if (leaving_var < 0 || entering_nb_pos < 0) {
        return false;
    }

    // Replace leaving basic variable with entering variable
    basic_vars[basic_pos] = entering_var;
    nonbasic_vars[entering_nb_pos] = leaving_var;

    // Update status mapping
    var_to_basic_pos[entering_var] = static_cast<index_t>(basic_pos);
    var_to_basic_pos[leaving_var] = -1;

    var_to_nonbasic_pos[leaving_var] = entering_nb_pos;
    var_to_nonbasic_pos[entering_var] = -1;

    var_status[entering_var] = VariableStatus::BASIC;
    var_status[leaving_var] = VariableStatus::NONBASIC_LOWER;

    return check_invariants();
}

bool Basis::check_invariants() const {
    if (basic_vars.size() != num_rows || nonbasic_vars.size() != (num_cols - num_rows)) {
        return false;
    }

    std::vector<bool> seen(num_cols, false);

    for (size_t pos = 0; pos < num_rows; ++pos) {
        index_t var = basic_vars[pos];
        if (var < 0 || static_cast<size_t>(var) >= num_cols || seen[var]) {
            return false;
        }
        seen[var] = true;
        if (var_to_basic_pos[var] != static_cast<index_t>(pos)) return false;
        if (var_status[var] != VariableStatus::BASIC) return false;
    }

    for (size_t nb_pos = 0; nb_pos < nonbasic_vars.size(); ++nb_pos) {
        index_t var = nonbasic_vars[nb_pos];
        if (var < 0 || static_cast<size_t>(var) >= num_cols || seen[var]) {
            return false;
        }
        seen[var] = true;
        if (var_to_nonbasic_pos[var] != static_cast<index_t>(nb_pos)) return false;
        if (var_status[var] == VariableStatus::BASIC) return false;
    }

    return true;
}

// ============================================================================
// DENSE BASIS SOLVER IMPLEMENTATION (LU DECOMPOSITION WITH PARTIAL PIVOTING)
// ============================================================================

bool DenseBasisSolver::factorize(const StandardFormLP& lp, const Basis& basis) {
    m_ = basis.num_rows;
    LU_.assign(m_, std::vector<real_t>(m_, 0.0));
    pivot_perm_.resize(m_);
    std::iota(pivot_perm_.begin(), pivot_perm_.end(), 0);

    // Extract columns B from lp.A corresponding to basic_vars
    for (size_t j = 0; j < m_; ++j) {
        index_t bvar = basis.basic_vars[j];
        for (size_t i = 0; i < m_; ++i) {
            LU_[i][j] = lp.A[i][bvar];
        }
    }

    // Perform LU decomposition with row partial pivoting
    for (size_t k = 0; k < m_; ++k) {
        // Find pivot element in column k
        size_t max_row = k;
        real_t max_val = std::abs(LU_[k][k]);

        for (size_t i = k + 1; i < m_; ++i) {
            real_t val = std::abs(LU_[i][k]);
            if (val > max_val) {
                max_val = val;
                max_row = i;
            }
        }

        if (max_val < DEFAULT_PIVOT_TOLERANCE) {
            return false; // Singular basis matrix B
        }

        if (max_row != k) {
            std::swap(LU_[k], LU_[max_row]);
            std::swap(pivot_perm_[k], pivot_perm_[max_row]);
        }

        // Eliminate column entries below diagonal
        for (size_t i = k + 1; i < m_; ++i) {
            LU_[i][k] /= LU_[k][k];
            for (size_t j = k + 1; j < m_; ++j) {
                LU_[i][j] -= LU_[i][k] * LU_[k][j];
            }
        }
    }

    return true;
}

bool DenseBasisSolver::solve_primal(const std::vector<real_t>& rhs, std::vector<real_t>& x) const {
    if (rhs.size() != m_) return false;
    x.resize(m_);

    // Forward substitution for L y = P rhs
    std::vector<real_t> y(m_);
    for (size_t i = 0; i < m_; ++i) {
        real_t sum = rhs[pivot_perm_[i]];
        for (size_t j = 0; j < i; ++j) {
            sum -= LU_[i][j] * y[j];
        }
        y[i] = sum;
    }

    // Back substitution for U x = y
    for (int i = static_cast<int>(m_) - 1; i >= 0; --i) {
        real_t sum = y[i];
        for (size_t j = i + 1; j < m_; ++j) {
            sum -= LU_[i][j] * x[j];
        }
        if (std::abs(LU_[i][i]) < DEFAULT_ZERO_TOLERANCE) return false;
        x[i] = sum / LU_[i][i];
    }

    return true;
}

bool DenseBasisSolver::solve_dual(const std::vector<real_t>& rhs, std::vector<real_t>& y) const {
    if (rhs.size() != m_) return false;
    y.resize(m_);

    // Solve B^T y = rhs => U^T L^T P y = rhs
    // Step 1: Solve U^T z = rhs
    std::vector<real_t> z(m_);
    for (size_t i = 0; i < m_; ++i) {
        real_t sum = rhs[i];
        for (size_t j = 0; j < i; ++j) {
            sum -= LU_[j][i] * z[j]; // Accessing U^T (LU_[j][i])
        }
        if (std::abs(LU_[i][i]) < DEFAULT_ZERO_TOLERANCE) return false;
        z[i] = sum / LU_[i][i];
    }

    // Step 2: Solve L^T w = z
    std::vector<real_t> w(m_);
    for (int i = static_cast<int>(m_) - 1; i >= 0; --i) {
        real_t sum = z[i];
        for (size_t j = i + 1; j < m_; ++j) {
            sum -= LU_[j][i] * w[j]; // Accessing L^T (LU_[j][i] with unit diagonal)
        }
        w[i] = sum;
    }

    // Step 3: Permute back y = P^T w
    for (size_t i = 0; i < m_; ++i) {
        y[pivot_perm_[i]] = w[i];
    }

    return true;
}

// ============================================================================
// REVISED SIMPLEX ENGINE IMPLEMENTATION
// ============================================================================

RevisedSimplex::RevisedSimplex(RevisedSimplexOptions options)
    : options_(options) {}

StandardFormLP RevisedSimplex::create_standard_form(const LPModel& model) const {
    StandardFormLP std_lp;
    std_lp.orig_sense = model.sense();
    std_lp.c0 = model.obj_offset();
    std_lp.orig_vars = model.num_variables();

    std_lp.var_shifts.assign(std_lp.orig_vars, 0.0);

    std::vector<std::string> var_col_names;
    std::vector<index_t> col_to_orig;
    std::vector<real_t> col_obj_coeffs;

    // Minimization factor: if MAXIMIZE, convert to MINIMIZE by c = -c
    real_t sense_factor = (model.sense() == ObjectiveSense::MINIMIZE) ? 1.0 : -1.0;

    for (size_t j = 0; j < std_lp.orig_vars; ++j) {
        const auto& var = model.get_variable(static_cast<index_t>(j));
        real_t lb = var.lower_bound;

        if (var.is_free()) {
            std_lp.var_shifts[j] = 0.0;
            var_col_names.push_back(var.name + "+");
            col_to_orig.push_back(static_cast<index_t>(j));
            col_obj_coeffs.push_back(var.obj_coeff * sense_factor);

            var_col_names.push_back(var.name + "-");
            col_to_orig.push_back(static_cast<index_t>(j));
            col_obj_coeffs.push_back(-var.obj_coeff * sense_factor);
        } else {
            real_t shift = 0.0;
            if (!std::isinf(lb) && lb != 0.0 && lb > -BHARATOPT_INFINITY) {
                shift = lb;
            }
            std_lp.var_shifts[j] = shift;
            var_col_names.push_back(var.name);
            col_to_orig.push_back(static_cast<index_t>(j));
            col_obj_coeffs.push_back(var.obj_coeff * sense_factor);
        }
    }

    struct StandardRow {
        std::vector<std::pair<size_t, real_t>> coeffs;
        real_t rhs{0.0};
        ConstraintSense sense{ConstraintSense::LESS_EQUAL};
        std::string name;
    };

    std::vector<StandardRow> std_rows;

    for (size_t i = 0; i < model.num_constraints(); ++i) {
        const auto& cons = model.get_constraint(static_cast<index_t>(i));

        auto process_single_row = [&](ConstraintSense sense, real_t raw_rhs, const std::string& row_name) {
            StandardRow srow;
            srow.sense = sense;
            srow.rhs = raw_rhs;
            srow.name = row_name;

            for (const auto& term : cons.terms) {
                index_t orig_j = term.first;
                real_t val = term.second;
                const auto& var = model.get_variable(orig_j);

                if (var.is_free()) {
                    size_t p_col = 0, m_col = 0;
                    for (size_t c = 0; c < col_to_orig.size(); ++c) {
                        if (col_to_orig[c] == orig_j) {
                            if (var_col_names[c].back() == '+') p_col = c;
                            else if (var_col_names[c].back() == '-') m_col = c;
                        }
                    }
                    srow.coeffs.push_back({p_col, val});
                    srow.coeffs.push_back({m_col, -val});
                } else {
                    size_t std_c = 0;
                    for (size_t c = 0; c < col_to_orig.size(); ++c) {
                        if (col_to_orig[c] == orig_j) {
                            std_c = c;
                            break;
                        }
                    }
                    srow.coeffs.push_back({std_c, val});
                    if (std_lp.var_shifts[orig_j] != 0.0) {
                        srow.rhs -= val * std_lp.var_shifts[orig_j];
                    }
                }
            }
            std_rows.push_back(srow);
        };

        if (cons.sense == ConstraintSense::RANGED) {
            process_single_row(ConstraintSense::GREATER_EQUAL, cons.rhs, cons.name + "_lower");
            process_single_row(ConstraintSense::LESS_EQUAL, cons.range_upper, cons.name + "_upper");
        } else {
            process_single_row(cons.sense, cons.rhs, cons.name);
        }
    }

    // Explicit upper bound constraints for finite u_j
    for (size_t j = 0; j < std_lp.orig_vars; ++j) {
        const auto& var = model.get_variable(static_cast<index_t>(j));
        if (!var.is_free() && var.upper_bound < BHARATOPT_INFINITY) {
            StandardRow ub_row;
            ub_row.sense = ConstraintSense::LESS_EQUAL;
            ub_row.rhs = var.upper_bound - std_lp.var_shifts[j];
            ub_row.name = var.name + "_ub";

            size_t std_c = 0;
            for (size_t c = 0; c < col_to_orig.size(); ++c) {
                if (col_to_orig[c] == static_cast<index_t>(j)) {
                    std_c = c;
                    break;
                }
            }
            ub_row.coeffs.push_back({std_c, 1.0});
            std_rows.push_back(ub_row);
        }
    }

    std_lp.num_rows = std_rows.size();
    size_t decision_cols = var_col_names.size();

    std_lp.col_names = var_col_names;
    std_lp.col_to_orig_var = col_to_orig;
    std_lp.c = col_obj_coeffs;

    std_lp.is_slack.assign(decision_cols, false);
    std_lp.is_surplus.assign(decision_cols, false);
    std_lp.is_artificial.assign(decision_cols, false);

    struct AuxCol {
        std::string name;
        size_t row_idx;
        bool is_slack{false};
        bool is_surplus{false};
        bool is_artificial{false};
    };

    std::vector<AuxCol> aux_cols;

    for (size_t i = 0; i < std_rows.size(); ++i) {
        auto& srow = std_rows[i];
        if (srow.rhs < 0.0) {
            srow.rhs = -srow.rhs;
            for (auto& term : srow.coeffs) term.second = -term.second;
            if (srow.sense == ConstraintSense::LESS_EQUAL) srow.sense = ConstraintSense::GREATER_EQUAL;
            else if (srow.sense == ConstraintSense::GREATER_EQUAL) srow.sense = ConstraintSense::LESS_EQUAL;
        }

        if (srow.sense == ConstraintSense::LESS_EQUAL) {
            aux_cols.push_back({"s_" + srow.name, i, true, false, false});
        } else if (srow.sense == ConstraintSense::GREATER_EQUAL) {
            aux_cols.push_back({"sur_" + srow.name, i, false, true, false});
            aux_cols.push_back({"art_" + srow.name, i, false, false, true});
        } else if (srow.sense == ConstraintSense::EQUAL) {
            aux_cols.push_back({"art_" + srow.name, i, false, false, true});
        }
    }

    for (const auto& aux : aux_cols) {
        std_lp.col_names.push_back(aux.name);
        std_lp.col_to_orig_var.push_back(-1);
        std_lp.c.push_back(0.0);
        std_lp.is_slack.push_back(aux.is_slack);
        std_lp.is_surplus.push_back(aux.is_surplus);
        std_lp.is_artificial.push_back(aux.is_artificial);
    }

    std_lp.num_cols = std_lp.col_names.size();

    // Construct dense 2D matrix A and vector b
    std_lp.A.assign(std_lp.num_rows, std::vector<real_t>(std_lp.num_cols, 0.0));
    std_lp.b.assign(std_lp.num_rows, 0.0);

    for (size_t i = 0; i < std_rows.size(); ++i) {
        const auto& srow = std_rows[i];
        for (const auto& term : srow.coeffs) {
            std_lp.A[i][term.first] = term.second;
        }
        std_lp.b[i] = srow.rhs;
    }

    size_t aux_idx = decision_cols;
    for (const auto& aux : aux_cols) {
        if (aux.is_slack) std_lp.A[aux.row_idx][aux_idx] = 1.0;
        else if (aux.is_surplus) std_lp.A[aux.row_idx][aux_idx] = -1.0;
        else if (aux.is_artificial) std_lp.A[aux.row_idx][aux_idx] = 1.0;
        aux_idx++;
    }

    // Adjust objective constant for shifts
    for (size_t j = 0; j < std_lp.orig_vars; ++j) {
        if (std_lp.var_shifts[j] != 0.0) {
            std_lp.c0 += model.get_variable(static_cast<index_t>(j)).obj_coeff * std_lp.var_shifts[j];
        }
    }

    return std_lp;
}

bool RevisedSimplex::compute_basic_solution(IBasisSolver& solver,
                                            const StandardFormLP& lp,
                                            const Basis& basis,
                                            std::vector<real_t>& x_B) const {
    (void)basis;
    return solver.solve_primal(lp.b, x_B);
}

bool RevisedSimplex::compute_dual_vector(IBasisSolver& solver,
                                         const StandardFormLP& lp,
                                         const Basis& basis,
                                         std::vector<real_t>& y) const {
    std::vector<real_t> c_B(lp.num_rows);
    for (size_t i = 0; i < lp.num_rows; ++i) {
        c_B[i] = lp.c[basis.basic_vars[i]];
    }
    return solver.solve_dual(c_B, y);
}

void RevisedSimplex::compute_reduced_costs(const StandardFormLP& lp,
                                           const std::vector<real_t>& y,
                                           std::vector<real_t>& r) const {
    r.resize(lp.num_cols);
    for (size_t j = 0; j < lp.num_cols; ++j) {
        real_t y_dot_Aj = 0.0;
        for (size_t i = 0; i < lp.num_rows; ++i) {
            y_dot_Aj += y[i] * lp.A[i][j];
        }
        r[j] = lp.c[j] - y_dot_Aj;
    }
}

index_t RevisedSimplex::select_entering_variable(const Basis& basis,
                                                 const std::vector<real_t>& r,
                                                 const StandardFormLP& lp) const {
    index_t entering_var = -1;
    real_t min_reduced_cost = -options_.optimality_tolerance;

    if (options_.entering_rule == EnteringRule::BLANDS_RULE) {
        for (index_t var : basis.nonbasic_vars) {
            if (lp.is_artificial[var] && lp.c[var] >= BHARATOPT_INFINITY / 2.0) continue;
            if (r[var] < -options_.optimality_tolerance) {
                if (entering_var < 0 || var < entering_var) {
                    entering_var = var;
                }
            }
        }
    } else { // Dantzig MOST_NEGATIVE
        for (index_t var : basis.nonbasic_vars) {
            if (lp.is_artificial[var] && lp.c[var] >= BHARATOPT_INFINITY / 2.0) continue;
            if (r[var] < min_reduced_cost) {
                min_reduced_cost = r[var];
                entering_var = var;
            }
        }
    }

    return entering_var;
}

bool RevisedSimplex::compute_direction(IBasisSolver& solver,
                                       const StandardFormLP& lp,
                                       index_t entering_var,
                                       std::vector<real_t>& d_B) const {
    std::vector<real_t> A_q(lp.num_rows);
    for (size_t i = 0; i < lp.num_rows; ++i) {
        A_q[i] = lp.A[i][entering_var];
    }
    return solver.solve_primal(A_q, d_B);
}

index_t RevisedSimplex::ratio_test(const std::vector<real_t>& x_B,
                                   const std::vector<real_t>& d_B,
                                   const Basis& basis,
                                   real_t& min_ratio) const {
    index_t leaving_pos = -1;
    min_ratio = BHARATOPT_INFINITY;

    for (size_t i = 0; i < basis.num_rows; ++i) {
        if (d_B[i] > options_.pivot_tolerance) {
            real_t val = x_B[i];
            if (std::abs(val) < options_.zero_tolerance) {
                val = 0.0;
            }
            real_t ratio = val / d_B[i];
            if (ratio < min_ratio - options_.zero_tolerance) {
                min_ratio = ratio;
                leaving_pos = static_cast<index_t>(i);
            } else if (std::abs(ratio - min_ratio) <= options_.zero_tolerance && leaving_pos >= 0) {
                // Bland tie-breaking: smallest basic variable index
                if (basis.basic_vars[i] < basis.basic_vars[leaving_pos]) {
                    leaving_pos = static_cast<index_t>(i);
                }
            }
        }
    }

    return leaving_pos;
}

RevisedSimplexResult RevisedSimplex::solve(const LPModel& model) {
    RevisedSimplexResult result;
    StandardFormLP lp = create_standard_form(model);

    // Initial Basis Identification
    std::vector<index_t> initial_basic(lp.num_rows, -1);
    size_t decision_cols = 0;
    for (size_t j = 0; j < lp.num_cols; ++j) {
        if (!lp.is_slack[j] && !lp.is_surplus[j] && !lp.is_artificial[j]) {
            decision_cols++;
        }
    }

    size_t aux_col = decision_cols;
    (void)aux_col;
    for (size_t i = 0; i < lp.num_rows; ++i) {
        // Find slack or artificial column for row i
        for (size_t j = decision_cols; j < lp.num_cols; ++j) {
            if (lp.A[i][j] == 1.0 && (lp.is_slack[j] || lp.is_artificial[j])) {
                initial_basic[i] = static_cast<index_t>(j);
                break;
            }
        }
    }

    Basis basis(lp.num_rows, lp.num_cols);
    if (!basis.set_initial_basis(initial_basic)) {
        result.status = RevisedSimplexStatus::UNSUPPORTED_INITIAL_BASIS;
        result.message = "Failed to construct valid initial basis";
        return result;
    }

    BasisUpdateManager* update_mgr = nullptr;
    std::unique_ptr<IBasisSolver> basis_solver;
    if (options_.enable_incremental_updates) {
        BasisUpdateOptions update_opts;
        update_opts.max_eta_updates = options_.max_eta_updates;
        update_opts.pivot_tolerance = options_.pivot_tolerance;
        update_opts.enable_incremental = true;
        auto base_solver = (options_.solver_type == BasisSolverType::SPARSE_LU) ?
            std::unique_ptr<IBasisSolver>(std::make_unique<SparseLUBasisSolver>(options_.pivot_tolerance)) :
            std::unique_ptr<IBasisSolver>(std::make_unique<DenseBasisSolver>());
        auto mgr = std::make_unique<BasisUpdateManager>(std::move(base_solver), update_opts);
        update_mgr = mgr.get();
        basis_solver = std::move(mgr);
    } else {
        if (options_.solver_type == BasisSolverType::SPARSE_LU) {
            basis_solver = std::make_unique<SparseLUBasisSolver>(options_.pivot_tolerance);
        } else {
            basis_solver = std::make_unique<DenseBasisSolver>();
        }
    }

    if (!basis_solver->factorize(lp, basis)) {
        result.status = RevisedSimplexStatus::NUMERICAL_FAILURE;
        result.message = "Initial basis matrix factorization failed";
        return result;
    }

    bool has_artificials = false;
    for (bool art : lp.is_artificial) {
        if (art) { has_artificials = true; break; }
    }

    std::vector<real_t> phase2_c = lp.c;

    if (has_artificials) {
        // --- PHASE I REVISED SIMPLEX ---
        // Objective: Minimize sum of artificials -> c_phase1 = 1.0 for artificials, 0.0 others
        std::fill(lp.c.begin(), lp.c.end(), 0.0);
        for (size_t j = 0; j < lp.num_cols; ++j) {
            if (lp.is_artificial[j]) lp.c[j] = 1.0;
        }

        while (result.iterations < options_.max_iterations) {
            std::vector<real_t> x_B, y, r, d_B;
            if (!compute_basic_solution(*basis_solver, lp, basis, x_B) ||
                !compute_dual_vector(*basis_solver, lp, basis, y)) {
                result.status = RevisedSimplexStatus::NUMERICAL_FAILURE;
                result.message = "Basic solution/dual vector solve failed during Phase I";
                return result;
            }

            compute_reduced_costs(lp, y, r);
            index_t enter_var = select_entering_variable(basis, r, lp);
            if (enter_var < 0) {
                break; // Phase I Optimal
            }

            if (!compute_direction(*basis_solver, lp, enter_var, d_B)) {
                result.status = RevisedSimplexStatus::NUMERICAL_FAILURE;
                result.message = "Direction computation failed during Phase I";
                return result;
            }

            real_t min_ratio = 0.0;
            index_t leave_pos = ratio_test(x_B, d_B, basis, min_ratio);
            if (leave_pos < 0) {
                result.status = RevisedSimplexStatus::UNBOUNDED;
                result.message = "Unbounded ray detected during Phase I";
                return result;
            }

            index_t leave_var = basis.basic_vars[leave_pos];
            bool update_ok = false;
            if (update_mgr) {
                update_ok = update_mgr->add_update(static_cast<size_t>(leave_pos), enter_var, leave_var, d_B);
            }

            if (!basis.update_basis(leave_pos, enter_var)) {
                result.status = RevisedSimplexStatus::NUMERICAL_FAILURE;
                result.message = "Basis update failed during Phase I";
                return result;
            }

            if (!update_ok) {
                if (!basis_solver->factorize(lp, basis)) {
                    result.status = RevisedSimplexStatus::NUMERICAL_FAILURE;
                    result.message = "Basis matrix factorization failed during Phase I";
                    return result;
                }
            }

            result.iterations++;
        }

        // Verify Phase I Feasibility
        if (!basis_solver->factorize(lp, basis)) {
            result.status = RevisedSimplexStatus::NUMERICAL_FAILURE;
            return result;
        }
        std::vector<real_t> x_B;
        compute_basic_solution(*basis_solver, lp, basis, x_B);

        real_t artificial_sum = 0.0;
        for (size_t i = 0; i < basis.num_rows; ++i) {
            if (lp.is_artificial[basis.basic_vars[i]]) {
                artificial_sum += x_B[i];
            }
        }

        if (artificial_sum > options_.feasibility_tolerance) {
            result.status = RevisedSimplexStatus::INFEASIBLE;
            result.message = "Infeasible LP: Phase I artificial variable sum > 0";
            return result;
        }

        // Restore Phase II objective coefficients and block artificials
        lp.c = phase2_c;
        for (size_t j = 0; j < lp.num_cols; ++j) {
            if (lp.is_artificial[j]) lp.c[j] = BHARATOPT_INFINITY;
        }
    }

    // --- PHASE II REVISED SIMPLEX ---
    if (!basis_solver->factorize(lp, basis)) {
        result.status = RevisedSimplexStatus::NUMERICAL_FAILURE;
        result.message = "Basis factorization failed before Phase II";
        return result;
    }

    while (result.iterations < options_.max_iterations) {
        std::vector<real_t> x_B, y, r, d_B;
        if (!compute_basic_solution(*basis_solver, lp, basis, x_B) ||
            !compute_dual_vector(*basis_solver, lp, basis, y)) {
            result.status = RevisedSimplexStatus::NUMERICAL_FAILURE;
            result.message = "Basic solution solve failed during Phase II";
            return result;
        }

        compute_reduced_costs(lp, y, r);
        index_t enter_var = select_entering_variable(basis, r, lp);
        if (enter_var < 0) {
            result.status = RevisedSimplexStatus::OPTIMAL;
            result.message = "Optimal solution found";
            result.dual_solution = y;
            result.reduced_costs = r;
            break;
        }

        if (!compute_direction(*basis_solver, lp, enter_var, d_B)) {
            result.status = RevisedSimplexStatus::NUMERICAL_FAILURE;
            result.message = "Direction computation failed during Phase II";
            return result;
        }

        real_t min_ratio = 0.0;
        index_t leave_pos = ratio_test(x_B, d_B, basis, min_ratio);
        if (leave_pos < 0) {
            result.status = RevisedSimplexStatus::UNBOUNDED;
            result.message = "Unbounded LP: entering variable has no positive direction components";
            return result;
        }

        index_t leave_var = basis.basic_vars[leave_pos];
        bool update_ok = false;
        if (update_mgr) {
            update_ok = update_mgr->add_update(static_cast<size_t>(leave_pos), enter_var, leave_var, d_B);
        }

        if (!basis.update_basis(leave_pos, enter_var)) {
            result.status = RevisedSimplexStatus::NUMERICAL_FAILURE;
            result.message = "Basis update failed during Phase II";
            return result;
        }

        if (!update_ok) {
            if (!basis_solver->factorize(lp, basis)) {
                result.status = RevisedSimplexStatus::NUMERICAL_FAILURE;
                result.message = "Basis factorization failed during Phase II";
                return result;
            }
        }

        result.iterations++;
    }

    if (result.iterations >= options_.max_iterations && result.status != RevisedSimplexStatus::OPTIMAL) {
        result.status = RevisedSimplexStatus::ITERATION_LIMIT;
        result.message = "Iteration limit reached before optimality";
    }

    result.final_basis = basis;

    // --- SOLUTION EXTRACTION ---
    if (result.status == RevisedSimplexStatus::OPTIMAL) {
        std::vector<real_t> x_B;
        compute_basic_solution(*basis_solver, lp, basis, x_B);

        result.primal_solution.assign(lp.orig_vars, 0.0);

        for (size_t i = 0; i < basis.num_rows; ++i) {
            index_t bvar = basis.basic_vars[i];
            index_t orig_j = lp.col_to_orig_var[bvar];
            if (orig_j >= 0 && static_cast<size_t>(orig_j) < lp.orig_vars) {
                real_t val = x_B[i];
                const auto& var = model.get_variable(orig_j);

                if (var.is_free()) {
                    if (lp.col_names[bvar].back() == '+') result.primal_solution[orig_j] += val;
                    else if (lp.col_names[bvar].back() == '-') result.primal_solution[orig_j] -= val;
                } else {
                    result.primal_solution[orig_j] = val + lp.var_shifts[orig_j];
                }
            }
        }

        // Assign shifted lower bounds to non-basic decision variables
        for (size_t j = 0; j < lp.orig_vars; ++j) {
            const auto& var = model.get_variable(static_cast<index_t>(j));
            if (!var.is_free() && lp.var_shifts[j] != 0.0) {
                bool is_in_basis = false;
                for (size_t i = 0; i < basis.num_rows; ++i) {
                    if (lp.col_to_orig_var[basis.basic_vars[i]] == static_cast<index_t>(j)) {
                        is_in_basis = true;
                        break;
                    }
                }
                if (!is_in_basis) {
                    result.primal_solution[j] = lp.var_shifts[j];
                }
            }
        }

        result.objective_value = recompute_original_objective(model, result.primal_solution);
    }

    return result;
}

bool RevisedSimplex::verify_solution_feasibility(const LPModel& model,
                                                 const std::vector<real_t>& solution,
                                                 real_t tol) const {
    if (solution.size() != model.num_variables()) return false;

    for (size_t j = 0; j < model.num_variables(); ++j) {
        const auto& var = model.get_variable(static_cast<index_t>(j));
        real_t val = solution[j];
        if (val < var.lower_bound - tol || val > var.upper_bound + tol) return false;
    }

    for (size_t i = 0; i < model.num_constraints(); ++i) {
        const auto& cons = model.get_constraint(static_cast<index_t>(i));
        real_t lhs = 0.0;
        for (const auto& term : cons.terms) {
            lhs += term.second * solution[term.first];
        }

        if (cons.sense == ConstraintSense::LESS_EQUAL && lhs > cons.rhs + tol) return false;
        if (cons.sense == ConstraintSense::GREATER_EQUAL && lhs < cons.rhs - tol) return false;
        if (cons.sense == ConstraintSense::EQUAL && std::abs(lhs - cons.rhs) > tol) return false;
        if (cons.sense == ConstraintSense::RANGED && (lhs < cons.rhs - tol || lhs > cons.range_upper + tol)) return false;
    }

    return true;
}

real_t RevisedSimplex::recompute_original_objective(const LPModel& model,
                                                     const std::vector<real_t>& solution) const {
    real_t obj = model.obj_offset();
    for (size_t j = 0; j < model.num_variables(); ++j) {
        obj += model.get_variable(static_cast<index_t>(j)).obj_coeff * solution[j];
    }
    return obj;
}

} // namespace bharatopt
