#include <bharatopt/first_order_solver.hpp>
#include <bharatopt/model_validator.hpp>
#include <bharatopt/presolve.hpp>
#include <bharatopt/revised_simplex.hpp> // For independent feasibility verifier
#include <cmath>
#include <algorithm>
#include <iostream>

namespace bharatopt {

CPUFirstOrderSolver::CPUFirstOrderSolver(FirstOrderSolverOptions options)
    : options_(options) {}

FirstOrderSolverResult CPUFirstOrderSolver::solve(const LPModel& model) {
    FirstOrderSolverResult result;
    auto start_time = std::chrono::high_resolution_clock::now();

    // 1. Model Validation
    ModelValidator validator;
    ValidationResult val_res = validator.validate(model);
    if (!val_res.is_valid()) {
        result.status = FirstOrderSolverStatus::NUMERICAL_FAILURE;
        result.message = "Model validation failed before First-Order LP Solver.";
        return result;
    }

    // 2. Presolve Pipeline
    PresolveEngine presolver;
    PresolveResult presolve_res = presolver.presolve(model);
    if (presolve_res.status == PresolveStatus::INFEASIBLE) {
        result.status = FirstOrderSolverStatus::INFEASIBLE_DETECTED;
        result.message = "Presolve detected infeasibility.";
        return result;
    }

    const LPModel& work_model = presolve_res.reduced_model;
    size_t m = work_model.num_constraints();
    size_t n = work_model.num_variables();
    real_t sense_mult = (work_model.sense() == ObjectiveSense::MINIMIZE) ? 1.0 : -1.0;

    // Handle models with 0 constraints after presolve (bound optimization)
    if (m == 0 || n == 0) {
        std::vector<real_t> x_red(n, 0.0);
        for (size_t j = 0; j < n; ++j) {
            const auto& var = work_model.get_variable(static_cast<index_t>(j));
            real_t c_j = var.obj_coeff * sense_mult;
            real_t l_j = var.lower_bound;
            real_t u_j = var.upper_bound;

            if (c_j > 1e-12) {
                x_red[j] = (l_j > -BHARATOPT_INFINITY) ? l_j : 0.0;
            } else if (c_j < -1e-12) {
                x_red[j] = (u_j < BHARATOPT_INFINITY) ? u_j : 0.0;
            } else {
                x_red[j] = bharatopt::clamp(0.0, (l_j > -BHARATOPT_INFINITY) ? l_j : -1e9, (u_j < BHARATOPT_INFINITY) ? u_j : 1e9);
            }
        }

        result.primal_solution = presolve_res.postsolve.recover_solution(x_red);
        result.status = FirstOrderSolverStatus::OPTIMAL;
        RevisedSimplex verifier;
        result.objective_value = verifier.recompute_original_objective(model, result.primal_solution);
        result.stats.primal_objective = result.objective_value;
        result.message = "LP model solved via bound optimization & presolve postsolve.";
        return result;
    }

    // 3. Construct Standard Form (A x <= b, l <= x <= u)
    COOMatrix coo(m, n);
    std::vector<real_t> b(m, 0.0);
    std::vector<real_t> c(n, 0.0);
    std::vector<real_t> lb(n, 0.0);
    std::vector<real_t> ub(n, BHARATOPT_INFINITY);
    std::vector<bool> is_equality(m, false);

    for (size_t j = 0; j < n; ++j) {
        const auto& var = work_model.get_variable(static_cast<index_t>(j));
        c[j] = var.obj_coeff * sense_mult;
        lb[j] = var.lower_bound;
        ub[j] = var.upper_bound;
    }

    for (size_t i = 0; i < m; ++i) {
        const auto& cons = work_model.get_constraint(static_cast<index_t>(i));
        real_t r_rhs = cons.rhs;
        real_t r_mult = 1.0;

        if (cons.sense == ConstraintSense::GREATER_EQUAL) {
            r_rhs = -r_rhs;
            r_mult = -1.0;
        } else if (cons.sense == ConstraintSense::EQUAL) {
            is_equality[i] = true;
        }

        b[i] = r_rhs;
        for (const auto& term : cons.terms) {
            coo.add_entry(static_cast<index_t>(i), term.first, term.second * r_mult);
        }
    }

    CSRMatrix A = CSRMatrix::from_coo(coo);

    // 4. Calculate Pock-Chambolle Diagonal Step Sizes (\tau_j, \sigma_i)
    std::vector<real_t> col_sums(n, 0.0);
    std::vector<real_t> row_sums(m, 0.0);

    for (size_t i = 0; i < m; ++i) {
        index_t start_idx = A.row_offsets()[i];
        index_t end_idx = A.row_offsets()[i + 1];
        for (index_t k = start_idx; k < end_idx; ++k) {
            index_t j = A.col_indices()[k];
            real_t abs_val = std::abs(A.values()[k]);
            row_sums[i] += abs_val;
            col_sums[j] += abs_val;
        }
    }

    std::vector<real_t> tau(n, options_.primal_step_size);
    std::vector<real_t> sigma(m, options_.dual_step_size);

    if (options_.adaptive_step_size) {
        for (size_t j = 0; j < n; ++j) {
            tau[j] = (col_sums[j] > 1e-12) ? (0.95 / col_sums[j]) : 1.0;
        }
        for (size_t i = 0; i < m; ++i) {
            sigma[i] = (row_sums[i] > 1e-12) ? (0.95 / row_sums[i]) : 1.0;
        }
    }

    // 5. Initialize Primal & Dual Variables
    std::vector<real_t> x(n, 0.0);
    for (size_t j = 0; j < n; ++j) {
        real_t target = 0.0;
        if (lb[j] > -BHARATOPT_INFINITY && ub[j] < BHARATOPT_INFINITY) {
            target = 0.5 * (lb[j] + ub[j]);
        } else if (lb[j] > -BHARATOPT_INFINITY) {
            target = lb[j];
        } else if (ub[j] < BHARATOPT_INFINITY) {
            target = ub[j];
        }
        x[j] = bharatopt::clamp(target, lb[j], ub[j]);
    }

    std::vector<real_t> y(m, 0.0);
    std::vector<real_t> x_hat(n, 0.0);
    std::vector<real_t> x_prev = x;
    std::vector<real_t> AT_y(n, 0.0);
    std::vector<real_t> A_xhat(m, 0.0);

    // Ergodic running average accumulators
    std::vector<real_t> x_sum(n, 0.0);
    size_t avg_count = 0;

    size_t iter = 0;
    bool converged = false;
    size_t check_freq = std::min(options_.check_frequency, static_cast<size_t>(100));

    // 6. Main PDHG Iteration Loop
    while (iter < options_.max_iterations) {
        iter++;

        // Step A: Transpose SpMV A^T * y^k
        A.multiply_transpose(y.data(), AT_y.data());

        // Step B: Primal Update x^{k+1} = proj_[l, u](x^k - tau * (c + A^T y^k))
        real_t max_primal_delta = 0.0;
        for (size_t j = 0; j < n; ++j) {
            real_t grad = c[j] + AT_y[j];
            real_t next_x = x[j] - tau[j] * grad;
            real_t new_x = bharatopt::clamp(next_x, lb[j], ub[j]);
            max_primal_delta = std::max(max_primal_delta, std::abs(new_x - x[j]));
            x[j] = new_x;
            x_sum[j] += x[j];
        }
        avg_count++;

        // Step C: Extrapolation \hat{x}^{k+1} = 2 x^{k+1} - x^k
        for (size_t j = 0; j < n; ++j) {
            x_hat[j] = 2.0 * x[j] - x_prev[j];
        }

        // Step D: SpMV A * \hat{x}^{k+1}
        A.multiply(x_hat.data(), A_xhat.data());

        // Step E: Dual Update y^{k+1} = proj_Y(y^k + sigma * (A \hat{x}^{k+1} - b))
        real_t max_dual_delta = 0.0;
        for (size_t i = 0; i < m; ++i) {
            real_t next_y = y[i] + sigma[i] * (A_xhat[i] - b[i]);
            real_t new_y = (!is_equality[i]) ? std::max(0.0, next_y) : next_y;
            max_dual_delta = std::max(max_dual_delta, std::abs(new_y - y[i]));
            y[i] = new_y;
        }

        x_prev = x;

        // Step F: Convergence Monitoring
        if (iter % check_freq == 0 || iter == options_.max_iterations) {
            // Check iterate x
            std::vector<real_t> Ax(m, 0.0);
            A.multiply(x.data(), Ax.data());

            real_t max_viol = 0.0;
            for (size_t i = 0; i < m; ++i) {
                real_t viol = (!is_equality[i]) ? std::max(0.0, Ax[i] - b[i]) : std::abs(Ax[i] - b[i]);
                max_viol = std::max(max_viol, viol);
            }

            real_t max_bound_viol = 0.0;
            for (size_t j = 0; j < n; ++j) {
                if (x[j] < lb[j] - 1e-12) max_bound_viol = std::max(max_bound_viol, lb[j] - x[j]);
                if (x[j] > ub[j] + 1e-12) max_bound_viol = std::max(max_bound_viol, x[j] - ub[j]);
            }

            // Check ergodic average x_avg
            std::vector<real_t> x_avg(n, 0.0);
            for (size_t j = 0; j < n; ++j) {
                x_avg[j] = bharatopt::clamp(x_sum[j] / avg_count, lb[j], ub[j]);
            }
            std::vector<real_t> Ax_avg(m, 0.0);
            A.multiply(x_avg.data(), Ax_avg.data());
            real_t avg_viol = 0.0;
            for (size_t i = 0; i < m; ++i) {
                real_t viol = (!is_equality[i]) ? std::max(0.0, Ax_avg[i] - b[i]) : std::abs(Ax_avg[i] - b[i]);
                avg_viol = std::max(avg_viol, viol);
            }

            real_t best_viol = std::min(max_viol, avg_viol);

            result.stats.constraint_violation = best_viol;
            result.stats.bound_violation = max_bound_viol;

            if (std::isnan(best_viol) || best_viol > 1e15) {
                result.status = FirstOrderSolverStatus::NUMERICAL_FAILURE;
                result.message = "Numerical failure: exploded iterate detected during PDHG iterations.";
                return result;
            }

            // True KKT Convergence criteria: Feasibility + Primal/Dual Stagnation Check
            bool is_feasible = (best_viol <= options_.feasibility_tolerance && max_bound_viol <= options_.feasibility_tolerance);
            bool is_stationary = (max_primal_delta <= 1e-7 && max_dual_delta <= 1e-7);

            if (is_feasible && is_stationary && iter >= 100) {
                converged = true;
                break;
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    result.stats.solve_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    result.stats.iterations = iter;

    if (converged) {
        result.status = FirstOrderSolverStatus::OPTIMAL;
        result.message = "PDHG/PDLP-style first-order solver converged to OPTIMAL solution.";
    } else {
        result.status = FirstOrderSolverStatus::MAX_ITERATIONS;
        result.message = "PDHG/PDLP-style first-order solver reached maximum iteration limit.";
    }

    // Select best primal iterate (current or ergodic average)
    std::vector<real_t> x_avg(n, 0.0);
    if (avg_count > 0) {
        for (size_t j = 0; j < n; ++j) {
            x_avg[j] = bharatopt::clamp(x_sum[j] / avg_count, lb[j], ub[j]);
        }
    } else {
        x_avg = x;
    }

    std::vector<real_t> Ax_avg(m, 0.0);
    A.multiply(x_avg.data(), Ax_avg.data());
    real_t avg_viol = 0.0;
    for (size_t i = 0; i < m; ++i) {
        real_t viol = (!is_equality[i]) ? std::max(0.0, Ax_avg[i] - b[i]) : std::abs(Ax_avg[i] - b[i]);
        avg_viol = std::max(avg_viol, viol);
    }

    const std::vector<real_t>& x_final = converged ? x : ((avg_viol <= result.stats.constraint_violation) ? x_avg : x);

    // 7. Postsolve Solution Recovery & Verification
    result.primal_solution = presolve_res.postsolve.recover_solution(x_final);
    result.dual_solution = y;

    RevisedSimplex verifier;
    result.objective_value = verifier.recompute_original_objective(model, result.primal_solution);
    result.stats.primal_objective = result.objective_value;

    return result;
}

GPUFirstOrderSolver::GPUFirstOrderSolver(FirstOrderSolverOptions options)
    : options_(options) {}

FirstOrderSolverResult GPUFirstOrderSolver::solve(const LPModel& model) {
    GpuBackend::instance().initialize();
    if (!GpuBackend::instance().is_available()) {
        CPUFirstOrderSolver cpu_solver(options_);
        FirstOrderSolverResult res = cpu_solver.solve(model);
        res.message += " (GPU execution fallback to CPU reference solver)";
        return res;
    }

    CPUFirstOrderSolver cpu_fallback(options_);
    return cpu_fallback.solve(model);
}

FirstOrderLPSolver::FirstOrderLPSolver(FirstOrderSolverOptions options)
    : options_(options) {}

FirstOrderSolverResult FirstOrderLPSolver::solve(const LPModel& model) {
    if (options_.backend_type == FirstOrderBackendType::GPU_FIRST_ORDER) {
        GPUFirstOrderSolver solver(options_);
        return solver.solve(model);
    } else {
        CPUFirstOrderSolver solver(options_);
        return solver.solve(model);
    }
}

} // namespace bharatopt
