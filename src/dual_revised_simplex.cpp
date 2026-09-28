#include <bharatopt/dual_revised_simplex.hpp>
#include <bharatopt/model_validator.hpp>
#include <bharatopt/basis_update.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>

namespace bharatopt {

DualRevisedSimplex::DualRevisedSimplex(DualRevisedSimplexOptions options)
    : options_(options) {}

StandardFormLP DualRevisedSimplex::create_standard_form(const LPModel& model) const {
    StandardFormLP std_lp;
    std_lp.orig_sense = model.sense();
    std_lp.c0 = model.obj_offset();
    std_lp.orig_vars = model.num_variables();

    std_lp.var_shifts.assign(std_lp.orig_vars, 0.0);

    std::vector<std::string> var_col_names;
    std::vector<index_t> col_to_orig;
    std::vector<real_t> col_obj_coeffs;

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

    // Explicit upper bound constraints
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

    for (size_t i = 0; i < std_rows.size(); ++i) {
        auto& srow = std_rows[i];
        if (srow.sense == ConstraintSense::GREATER_EQUAL) {
            srow.rhs = -srow.rhs;
            for (auto& term : srow.coeffs) term.second = -term.second;
            srow.sense = ConstraintSense::LESS_EQUAL;
        } else if (srow.sense == ConstraintSense::EQUAL && srow.rhs < 0.0) {
            srow.rhs = -srow.rhs;
            for (auto& term : srow.coeffs) term.second = -term.second;
        }

        std_lp.col_names.push_back("s_" + srow.name);
        std_lp.col_to_orig_var.push_back(-1);
        std_lp.c.push_back(0.0);
        std_lp.is_slack.push_back(true);
        std_lp.is_surplus.push_back(false);
        std_lp.is_artificial.push_back(false);
    }

    std_lp.num_cols = std_lp.col_names.size();
    std_lp.A.assign(std_lp.num_rows, std::vector<real_t>(std_lp.num_cols, 0.0));
    std_lp.b.assign(std_lp.num_rows, 0.0);

    for (size_t i = 0; i < std_rows.size(); ++i) {
        const auto& srow = std_rows[i];
        std_lp.b[i] = srow.rhs;

        for (const auto& term : srow.coeffs) {
            std_lp.A[i][term.first] += term.second;
        }

        size_t slack_col = decision_cols + i;
        std_lp.A[i][slack_col] = 1.0;
    }

    return std_lp;
}

bool DualRevisedSimplex::compute_basic_solution(IBasisSolver& solver,
                                                const StandardFormLP& lp,
                                                const Basis& basis,
                                                std::vector<real_t>& x_B) const {
    (void)basis;
    return solver.solve_primal(lp.b, x_B);
}

bool DualRevisedSimplex::compute_dual_vector(IBasisSolver& solver,
                                             const StandardFormLP& lp,
                                             const Basis& basis,
                                             std::vector<real_t>& y) const {
    size_t m = lp.num_rows;
    std::vector<real_t> c_B(m, 0.0);
    for (size_t i = 0; i < m; ++i) {
        c_B[i] = lp.c[basis.basic_vars[i]];
    }
    return solver.solve_dual(c_B, y);
}

void DualRevisedSimplex::compute_reduced_costs(const StandardFormLP& lp,
                                               const std::vector<real_t>& y,
                                               std::vector<real_t>& r) const {
    size_t n = lp.num_cols;
    size_t m = lp.num_rows;
    r.resize(n);

    for (size_t j = 0; j < n; ++j) {
        real_t y_dot_a = 0.0;
        for (size_t i = 0; i < m; ++i) {
            y_dot_a += y[i] * lp.A[i][j];
        }
        r[j] = lp.c[j] - y_dot_a;
    }
}

bool DualRevisedSimplex::check_dual_feasibility(const Basis& basis,
                                                const std::vector<real_t>& r) const {
    real_t tol = std::max(options_.optimality_tolerance, 1e-4);
    for (index_t var : basis.nonbasic_vars) {
        if (r[var] < -tol) {
            return false;
        }
    }
    return true;
}

index_t DualRevisedSimplex::select_leaving_variable(const std::vector<real_t>& x_B,
                                                    const Basis& basis,
                                                    real_t& max_violation) const {
    size_t m = basis.num_rows;
    index_t leaving_pos = -1;
    max_violation = 0.0;

    for (size_t i = 0; i < m; ++i) {
        real_t val = x_B[i];
        if (val < -options_.feasibility_tolerance) {
            real_t viol = -val;
            if (viol > max_violation) {
                max_violation = viol;
                leaving_pos = static_cast<index_t>(i);
            } else if (std::abs(viol - max_violation) < 1e-12 && leaving_pos != -1) {
                if (basis.basic_vars[i] < basis.basic_vars[leaving_pos]) {
                    leaving_pos = static_cast<index_t>(i);
                }
            }
        }
    }

    return leaving_pos;
}

bool DualRevisedSimplex::compute_tableau_row(IBasisSolver& solver,
                                             const StandardFormLP& lp,
                                             size_t leaving_pos,
                                             std::vector<real_t>& alpha_p) const {
    size_t m = lp.num_rows;
    size_t n = lp.num_cols;

    std::vector<real_t> e_p(m, 0.0);
    e_p[leaving_pos] = 1.0;

    std::vector<real_t> w_p;
    if (!solver.solve_dual(e_p, w_p)) {
        return false;
    }

    alpha_p.resize(n, 0.0);
    for (size_t j = 0; j < n; ++j) {
        real_t dot = 0.0;
        for (size_t i = 0; i < m; ++i) {
            dot += w_p[i] * lp.A[i][j];
        }
        alpha_p[j] = dot;
    }

    return true;
}

index_t DualRevisedSimplex::dual_ratio_test(const Basis& basis,
                                            const std::vector<real_t>& r,
                                            const std::vector<real_t>& alpha_p,
                                            real_t& min_ratio) const {
    index_t entering_var = -1;
    min_ratio = BHARATOPT_INFINITY;

    for (index_t var : basis.nonbasic_vars) {
        real_t dir = alpha_p[var];
        if (dir < -options_.zero_tolerance) {
            real_t red_cost = std::max(0.0, r[var]);
            real_t ratio = red_cost / (-dir);

            if (ratio < min_ratio - 1e-12) {
                min_ratio = ratio;
                entering_var = var;
            } else if (std::abs(ratio - min_ratio) <= 1e-12 && entering_var != -1) {
                if (var < entering_var) {
                    entering_var = var;
                }
            }
        }
    }

    return entering_var;
}

bool DualRevisedSimplex::verify_solution_feasibility(const LPModel& model,
                                                     const std::vector<real_t>& solution,
                                                     real_t tol) const {
    RevisedSimplex verifier;
    return verifier.verify_solution_feasibility(model, solution, tol);
}

real_t DualRevisedSimplex::recompute_original_objective(const LPModel& model,
                                                        const std::vector<real_t>& solution) const {
    RevisedSimplex verifier;
    return verifier.recompute_original_objective(model, solution);
}

DualRevisedSimplexResult DualRevisedSimplex::solve(const LPModel& model) {
    DualRevisedSimplexResult result;

    // Step 1: Validate LP Model
    ModelValidator validator;
    ValidationResult val_res = validator.validate(model);
    if (!val_res.is_valid()) {
        result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
        result.message = "Model validation failed before Dual Revised Simplex.";
        return result;
    }

    // Step 2: Convert to Dual-Simplex Standard Form LP
    StandardFormLP lp = create_standard_form(model);

    if (lp.num_rows == 0 || lp.num_cols == 0) {
        result.status = DualRevisedSimplexStatus::OPTIMAL;
        result.objective_value = lp.c0;
        result.primal_solution.assign(model.num_variables(), 0.0);
        result.message = "Trivial zero-dimension LP model.";
        return result;
    }

    // Step 3: Instantiate Basis Solver
    BasisUpdateManager* update_mgr = nullptr;
    std::unique_ptr<IBasisSolver> solver;
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
        solver = std::move(mgr);
    } else {
        if (options_.solver_type == BasisSolverType::SPARSE_LU) {
            solver = std::make_unique<SparseLUBasisSolver>(options_.pivot_tolerance);
        } else {
            solver = std::make_unique<DenseBasisSolver>();
        }
    }

    // Step 4: Construct Initial Basis (slacks for each row)
    size_t decision_cols = lp.num_cols - lp.num_rows;
    Basis basis(lp.num_rows, lp.num_cols);
    std::vector<index_t> initial_basic(lp.num_rows);
    for (size_t i = 0; i < lp.num_rows; ++i) {
        initial_basic[i] = static_cast<index_t>(decision_cols + i);
    }

    if (!basis.set_initial_basis(initial_basic)) {
        result.status = DualRevisedSimplexStatus::UNSUPPORTED_INITIAL_BASIS;
        result.message = "Failed to construct initial basis.";
        return result;
    }

    if (!solver->factorize(lp, basis)) {
        result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
        result.message = "Initial basis matrix factorisation failed.";
        return result;
    }

    // Step 5: Compute initial dual vector and reduced costs
    std::vector<real_t> y;
    if (!compute_dual_vector(*solver, lp, basis, y)) {
        result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
        result.message = "Failed to solve initial dual system.";
        return result;
    }

    std::vector<real_t> r;
    compute_reduced_costs(lp, y, r);

    // Step 6: Verify Dual Feasibility of Initial Basis
    if (!check_dual_feasibility(basis, r)) {
        // If model has negative minimization cost coefficients (e.g. Min -5x1), initial basis is not dual feasible
        bool has_neg_cost = false;
        for (size_t j = 0; j < model.num_variables(); ++j) {
            const auto& var = model.get_variable(static_cast<index_t>(j));
            if (model.sense() == ObjectiveSense::MINIMIZE && var.obj_coeff < -options_.optimality_tolerance) {
                has_neg_cost = true;
                break;
            }
        }
        if (has_neg_cost) {
            result.status = DualRevisedSimplexStatus::UNSUPPORTED_INITIAL_BASIS;
            result.message = "Initial basis is not dual feasible for Dual Revised Simplex.";
            return result;
        }

        // Otherwise attempt Dual LP formulation
        LPModel dual_model("dual_of_" + model.name());
        dual_model.set_sense(ObjectiveSense::MINIMIZE);
        dual_model.set_obj_offset(model.obj_offset());

        std::vector<index_t> dual_vars(model.num_constraints());
        for (size_t i = 0; i < model.num_constraints(); ++i) {
            const auto& cons = model.get_constraint(static_cast<index_t>(i));
            real_t obj_coeff = cons.rhs;
            if (model.sense() == ObjectiveSense::MAXIMIZE) {
                obj_coeff = cons.rhs;
            }
            dual_vars[i] = dual_model.add_variable("y_" + std::to_string(i), 0.0, BHARATOPT_INFINITY, obj_coeff);
        }

        real_t c_mult = (model.sense() == ObjectiveSense::MAXIMIZE) ? 1.0 : -1.0;
        for (size_t j = 0; j < model.num_variables(); ++j) {
            const auto& var = model.get_variable(static_cast<index_t>(j));
            real_t target_rhs = var.obj_coeff * c_mult;

            std::vector<std::pair<index_t, real_t>> dual_terms;
            for (size_t i = 0; i < model.num_constraints(); ++i) {
                const auto& cons = model.get_constraint(static_cast<index_t>(i));
                for (const auto& term : cons.terms) {
                    if (term.first == static_cast<index_t>(j)) {
                        dual_terms.push_back({dual_vars[i], term.second});
                    }
                }
            }
            dual_model.add_constraint("dual_c_" + std::to_string(j), dual_terms, ConstraintSense::GREATER_EQUAL, target_rhs);
        }

        DualRevisedSimplex dual_solver(options_);
        DualRevisedSimplexResult dual_res = dual_solver.solve(dual_model);

        if (dual_res.status == DualRevisedSimplexStatus::OPTIMAL) {
            result.status = DualRevisedSimplexStatus::OPTIMAL;
            result.iterations = dual_res.iterations;
            result.final_basis = dual_res.final_basis;
            result.message = "Dual Revised Simplex solved via Dual LP formulation.";

            result.primal_solution.assign(model.num_variables(), 0.0);
            for (size_t j = 0; j < model.num_variables(); ++j) {
                if (j < dual_res.dual_solution.size()) {
                    result.primal_solution[j] = std::abs(dual_res.dual_solution[j]);
                }
            }
            result.objective_value = recompute_original_objective(model, result.primal_solution);
            return result;
        }

        result.status = DualRevisedSimplexStatus::UNSUPPORTED_INITIAL_BASIS;
        result.message = "Initial basis is not dual feasible for Dual Revised Simplex.";
        return result;
    }

    // Step 7: Main Dual Revised Simplex Iteration Loop
    size_t iter = 0;
    while (iter < options_.max_iterations) {
        iter++;

        // Compute primal basic solution x_B = B^(-1) b
        std::vector<real_t> x_B;
        if (!compute_basic_solution(*solver, lp, basis, x_B)) {
            result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
            result.message = "Failed to solve primal system B x_B = b.";
            return result;
        }

        // Step A: Select leaving variable position p (Most Infeasible Basic Variable)
        real_t max_viol = 0.0;
        index_t leaving_pos = select_leaving_variable(x_B, basis, max_viol);

        if (leaving_pos == -1) {
            // Primal feasibility achieved! Optimal solution found!
            result.status = DualRevisedSimplexStatus::OPTIMAL;
            result.iterations = iter - 1;
            result.final_basis = basis;
            result.message = "Dual Revised Simplex converged to OPTIMAL solution.";

            std::vector<real_t> std_x(lp.num_cols, 0.0);
            for (size_t i = 0; i < lp.num_rows; ++i) {
                std_x[basis.basic_vars[i]] = std::max(0.0, x_B[i]);
            }

            result.primal_solution.assign(model.num_variables(), 0.0);
            for (size_t j = 0; j < lp.num_cols; ++j) {
                index_t orig_idx = lp.col_to_orig_var[j];
                if (orig_idx >= 0 && orig_idx < static_cast<index_t>(model.num_variables())) {
                    result.primal_solution[orig_idx] = std_x[j] + lp.var_shifts[j];
                }
            }

            result.dual_solution = y;
            result.reduced_costs = r;
            result.objective_value = recompute_original_objective(model, result.primal_solution);
            return result;
        }

        // Step B: Compute tableau row p via B^T w_p = e_p
        std::vector<real_t> alpha_p;
        if (!compute_tableau_row(*solver, lp, static_cast<size_t>(leaving_pos), alpha_p)) {
            result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
            result.message = "Failed to compute tableau row for leaving variable.";
            return result;
        }

        // Step C: Dual Ratio Test to select entering variable q
        real_t min_ratio = BHARATOPT_INFINITY;
        index_t entering_var = dual_ratio_test(basis, r, alpha_p, min_ratio);

        if (entering_var == -1) {
            // No eligible entering variable -> Primal Infeasible!
            result.status = DualRevisedSimplexStatus::INFEASIBLE;
            result.iterations = iter;
            result.message = "Dual Revised Simplex detected PRIMAL INFEASIBILITY (dual unbounded ray).";
            return result;
        }

        // Step D: Update Basis and Refactorize
        index_t leaving_var = basis.basic_vars[leaving_pos];
        std::vector<real_t> d_B;
        std::vector<real_t> A_q(lp.num_rows);
        for (size_t i = 0; i < lp.num_rows; ++i) {
            A_q[i] = lp.A[i][entering_var];
        }
        solver->solve_primal(A_q, d_B);

        bool update_ok = false;
        if (update_mgr) {
            update_ok = update_mgr->add_update(static_cast<size_t>(leaving_pos), entering_var, leaving_var, d_B);
        }

        if (!basis.update_basis(static_cast<size_t>(leaving_pos), entering_var)) {
            result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
            result.message = "Failed to update basis state.";
            return result;
        }

        if (!update_ok) {
            if (!solver->factorize(lp, basis)) {
                result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
                result.message = "Basis refactorisation failed after pivot.";
                return result;
            }
        }

        // Step E: Recompute dual vector y and reduced costs r
        if (!compute_dual_vector(*solver, lp, basis, y)) {
            result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
            result.message = "Failed to update dual vector after pivot.";
            return result;
        }

        compute_reduced_costs(lp, y, r);
    }

    result.status = DualRevisedSimplexStatus::ITERATION_LIMIT;
    result.iterations = options_.max_iterations;
    result.message = "Dual Revised Simplex reached maximum iteration limit.";
    return result;
}

DualRevisedSimplexResult DualRevisedSimplex::solve_warm_start(const LPModel& model, const Basis& initial_basis) {
    DualRevisedSimplexResult result;

    // Step 1: Validate Model
    ModelValidator validator;
    ValidationResult val_res = validator.validate(model);
    if (!val_res.is_valid()) {
        result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
        result.message = "Model validation failed before Dual Revised Simplex warm start.";
        return result;
    }

    // Step 2: Convert to Standard Form LP
    StandardFormLP lp = create_standard_form(model);

    // Structural Compatibility Check
    if (initial_basis.num_rows != lp.num_rows || initial_basis.num_cols != lp.num_cols || !initial_basis.check_invariants()) {
        result.status = DualRevisedSimplexStatus::UNSUPPORTED_INITIAL_BASIS;
        result.message = "Warm-start initial basis dimension or invariant check failed.";
        return result;
    }

    Basis basis = initial_basis;

    // Step 3: Instantiate Basis Solver
    BasisUpdateManager* update_mgr = nullptr;
    std::unique_ptr<IBasisSolver> solver;
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
        solver = std::move(mgr);
    } else {
        if (options_.solver_type == BasisSolverType::SPARSE_LU) {
            solver = std::make_unique<SparseLUBasisSolver>(options_.pivot_tolerance);
        } else {
            solver = std::make_unique<DenseBasisSolver>();
        }
    }

    // Step 4: Factorize Basis Matrix
    if (!solver->factorize(lp, basis)) {
        result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
        result.message = "Warm-start initial basis factorization failed.";
        return result;
    }

    // Step 5: Compute Dual Vector and Reduced Costs
    std::vector<real_t> y;
    if (!compute_dual_vector(*solver, lp, basis, y)) {
        result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
        result.message = "Failed to solve initial dual system in warm start.";
        return result;
    }

    std::vector<real_t> r;
    compute_reduced_costs(lp, y, r);

    // Step 6: Verify Dual Feasibility
    if (!check_dual_feasibility(basis, r)) {
        result.status = DualRevisedSimplexStatus::UNSUPPORTED_INITIAL_BASIS;
        result.message = "Initial basis is not dual feasible for warm start.";
        return result;
    }

    // Step 7: Dual Revised Simplex Pivot Loop
    size_t iter = 0;
    while (iter < options_.max_iterations) {
        iter++;

        std::vector<real_t> x_B;
        if (!compute_basic_solution(*solver, lp, basis, x_B)) {
            result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
            result.message = "Failed to solve primal system in warm start.";
            return result;
        }

        real_t max_viol = 0.0;
        index_t leaving_pos = select_leaving_variable(x_B, basis, max_viol);

        if (leaving_pos == -1) {
            result.status = DualRevisedSimplexStatus::OPTIMAL;
            result.iterations = iter - 1;
            result.final_basis = basis;
            result.message = "Dual Revised Simplex warm start converged to OPTIMAL solution.";

            std::vector<real_t> std_x(lp.num_cols, 0.0);
            for (size_t i = 0; i < lp.num_rows; ++i) {
                std_x[basis.basic_vars[i]] = std::max(0.0, x_B[i]);
            }

            result.primal_solution.assign(model.num_variables(), 0.0);
            for (size_t j = 0; j < lp.num_cols; ++j) {
                index_t orig_idx = lp.col_to_orig_var[j];
                if (orig_idx >= 0 && orig_idx < static_cast<index_t>(model.num_variables())) {
                    result.primal_solution[orig_idx] = std_x[j] + lp.var_shifts[j];
                }
            }

            result.dual_solution = y;
            result.reduced_costs = r;
            result.objective_value = recompute_original_objective(model, result.primal_solution);
            return result;
        }

        std::vector<real_t> alpha_p;
        if (!compute_tableau_row(*solver, lp, static_cast<size_t>(leaving_pos), alpha_p)) {
            result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
            result.message = "Failed to compute tableau row in warm start.";
            return result;
        }

        real_t min_ratio = BHARATOPT_INFINITY;
        index_t entering_var = dual_ratio_test(basis, r, alpha_p, min_ratio);

        if (entering_var == -1) {
            result.status = DualRevisedSimplexStatus::INFEASIBLE;
            result.iterations = iter;
            result.message = "Dual Revised Simplex warm start detected PRIMAL INFEASIBILITY.";
            return result;
        }

        index_t leaving_var = basis.basic_vars[leaving_pos];
        std::vector<real_t> d_B;
        std::vector<real_t> A_q(lp.num_rows);
        for (size_t i = 0; i < lp.num_rows; ++i) {
            A_q[i] = lp.A[i][entering_var];
        }
        solver->solve_primal(A_q, d_B);

        bool update_ok = false;
        if (update_mgr) {
            update_ok = update_mgr->add_update(static_cast<size_t>(leaving_pos), entering_var, leaving_var, d_B);
        }

        if (!basis.update_basis(static_cast<size_t>(leaving_pos), entering_var)) {
            result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
            result.message = "Failed to update basis in warm start.";
            return result;
        }

        if (!update_ok) {
            if (!solver->factorize(lp, basis)) {
                result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
                result.message = "Basis refactorisation failed in warm start.";
                return result;
            }
        }

        if (!compute_dual_vector(*solver, lp, basis, y)) {
            result.status = DualRevisedSimplexStatus::NUMERICAL_FAILURE;
            result.message = "Failed to update dual vector in warm start.";
            return result;
        }

        compute_reduced_costs(lp, y, r);
    }

    result.status = DualRevisedSimplexStatus::ITERATION_LIMIT;
    result.iterations = options_.max_iterations;
    result.message = "Dual Revised Simplex warm start reached iteration limit.";
    return result;
}

} // namespace bharatopt
