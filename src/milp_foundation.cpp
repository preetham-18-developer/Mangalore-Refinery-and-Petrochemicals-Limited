#include <bharatopt/milp_foundation.hpp>
#include <chrono>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace bharatopt {

std::string model_type_to_string(ModelType type) {
    switch (type) {
        case ModelType::LP: return "LP";
        case ModelType::MILP: return "MILP";
        case ModelType::UNSUPPORTED_MODEL: return "UNSUPPORTED_MODEL";
        default: return "UNKNOWN";
    }
}

std::string milp_solution_status_to_string(MilpSolutionStatus status) {
    switch (status) {
        case MilpSolutionStatus::LP_RELAXATION_OPTIMAL: return "LP_RELAXATION_OPTIMAL";
        case MilpSolutionStatus::INTEGER_FEASIBLE_SOLUTION: return "INTEGER_FEASIBLE_SOLUTION";
        case MilpSolutionStatus::INTEGER_INFEASIBLE_SOLUTION: return "INTEGER_INFEASIBLE_SOLUTION";
        case MilpSolutionStatus::NUMERICAL_FAILURE: return "NUMERICAL_FAILURE";
        case MilpSolutionStatus::UNSUPPORTED_MODEL: return "UNSUPPORTED_MODEL";
        default: return "UNKNOWN";
    }
}

std::string IntegerVerificationResult::to_string() const {
    std::ostringstream oss;
    oss << "Integer Verification: " << (is_integer_feasible ? "FEASIBLE" : "INFEASIBLE/FRACTIONAL") << "\n"
        << "  Violating Variable Count: " << violating_variable_count << "\n"
        << "  Max Integrality Violation: " << std::scientific << std::setprecision(4) << max_integrality_violation << "\n"
        << "  Max Bound Violation: " << max_bound_violation << "\n"
        << "  Max Constraint Residual: " << max_constraint_residual << "\n"
        << "  Constraint Feasibility Passed: " << (constraint_feasibility_passed ? "YES" : "NO") << "\n"
        << "  Bounds Passed: " << (bounds_passed ? "YES" : "NO") << "\n"
        << "  Diagnostics: " << diagnostics;
    return oss.str();
}

ModelType MilpFoundation::classify_model(const LPModel& model) {
    ModelValidator validator;
    ValidationResult val_res = validator.validate(model);
    if (!val_res.is_valid()) {
        return ModelType::UNSUPPORTED_MODEL;
    }

    size_t integer_count = 0;
    const auto& vars = model.variables();

    for (const auto& var : vars) {
        if (var.type == VariableType::INTEGER || var.type == VariableType::BINARY) {
            integer_count++;
        }
    }

    if (integer_count == 0) {
        return ModelType::LP;
    } else {
        return ModelType::MILP;
    }
}

LPRelaxation MilpFoundation::extract_relaxation(const LPModel& model) {
    LPRelaxation rel;
    rel.relaxation_model = LPModel(model.name() + "_lp_relaxation");
    rel.relaxation_model.set_sense(model.sense());
    rel.relaxation_model.set_obj_offset(model.obj_offset());

    const auto& vars = model.variables();
    rel.mapping.original_types.reserve(vars.size());
    rel.mapping.var_names.reserve(vars.size());
    rel.mapping.milp_to_lp.resize(vars.size());
    rel.mapping.lp_to_milp.resize(vars.size());

    for (size_t j = 0; j < vars.size(); ++j) {
        const auto& var = vars[j];
        index_t idx = static_cast<index_t>(j);

        real_t lb = var.lower_bound;
        real_t ub = var.upper_bound;

        if (var.type == VariableType::BINARY) {
            lb = std::max(static_cast<real_t>(0.0), lb);
            ub = std::min(static_cast<real_t>(1.0), ub);
        }

        index_t lp_idx = rel.relaxation_model.add_variable(
            var.name, lb, ub, var.obj_coeff, VariableType::CONTINUOUS
        );

        rel.mapping.milp_to_lp[idx] = lp_idx;
        rel.mapping.lp_to_milp[lp_idx] = idx;
        rel.mapping.original_types.push_back(var.type);
        rel.mapping.var_names.push_back(var.name);
    }

    const auto& constraints = model.constraints();
    for (const auto& cons : constraints) {
        rel.relaxation_model.add_constraint(
            cons.name, cons.terms, cons.sense, cons.rhs, cons.range_upper
        );
    }

    return rel;
}

IntegerVerificationResult MilpFoundation::verify_integer_feasibility(
    const LPModel& original_milp,
    const std::vector<real_t>& solution,
    real_t integrality_tolerance
) {
    IntegerVerificationResult res;
    res.is_integer_feasible = true;
    res.constraint_feasibility_passed = true;
    res.bounds_passed = true;

    const auto& vars = original_milp.variables();
    if (solution.size() < vars.size()) {
        res.is_integer_feasible = false;
        res.diagnostics = "Solution vector dimension smaller than variable count.";
        return res;
    }

    // 1. Check Bounds & Integrality
    for (size_t j = 0; j < vars.size(); ++j) {
        const auto& var = vars[j];
        real_t val = solution[j];

        // NaN / Inf check
        if (std::isnan(val) || std::isinf(val)) {
            res.is_integer_feasible = false;
            res.bounds_passed = false;
            res.violating_variable_count++;
            res.violating_var_indices.push_back(static_cast<index_t>(j));
            continue;
        }

        // Bound check
        real_t lb_viol = std::max(static_cast<real_t>(0.0), var.lower_bound - val);
        real_t ub_viol = std::max(static_cast<real_t>(0.0), val - var.upper_bound);
        real_t b_viol = std::max(lb_viol, ub_viol);

        if (b_viol > 1e-4) {
            res.bounds_passed = false;
            res.max_bound_violation = std::max(res.max_bound_violation, b_viol);
        }

        // Integrality check
        real_t integ_viol = 0.0;
        if (var.type == VariableType::INTEGER) {
            real_t nearest_int = std::round(val);
            integ_viol = std::abs(val - nearest_int);
        } else if (var.type == VariableType::BINARY) {
            real_t dist_0 = std::abs(val - 0.0);
            real_t dist_1 = std::abs(val - 1.0);
            integ_viol = std::min(dist_0, dist_1);
        }

        if (integ_viol > integrality_tolerance) {
            res.is_integer_feasible = false;
            res.violating_variable_count++;
            res.violating_var_indices.push_back(static_cast<index_t>(j));
            res.max_integrality_violation = std::max(res.max_integrality_violation, integ_viol);
        }
    }

    // 2. Check Constraints
    const auto& constraints = original_milp.constraints();
    for (const auto& cons : constraints) {
        real_t lhs = 0.0;
        for (const auto& term : cons.terms) {
            if (term.first >= 0 && static_cast<size_t>(term.first) < solution.size()) {
                lhs += term.second * solution[term.first];
            }
        }

        real_t residual = 0.0;
        if (cons.sense == ConstraintSense::LESS_EQUAL) {
            residual = std::max(static_cast<real_t>(0.0), lhs - cons.rhs);
        } else if (cons.sense == ConstraintSense::EQUAL) {
            residual = std::abs(lhs - cons.rhs);
        } else if (cons.sense == ConstraintSense::GREATER_EQUAL) {
            residual = std::max(static_cast<real_t>(0.0), cons.rhs - lhs);
        } else if (cons.sense == ConstraintSense::RANGED) {
            real_t lower_viol = std::max(static_cast<real_t>(0.0), cons.rhs - lhs);
            real_t upper_viol = std::max(static_cast<real_t>(0.0), lhs - cons.range_upper);
            residual = std::max(lower_viol, upper_viol);
        }

        if (residual > 1e-4) {
            res.constraint_feasibility_passed = false;
        }
        res.max_constraint_residual = std::max(res.max_constraint_residual, residual);
    }

    if (!res.constraint_feasibility_passed || !res.bounds_passed) {
        res.is_integer_feasible = false;
    }

    std::ostringstream diag;
    if (res.is_integer_feasible) {
        diag << "Solution satisfies constraint, bound, and integrality tolerances.";
    } else {
        diag << "Integrality/bound/constraint violation detected. ";
        if (res.violating_variable_count > 0) {
            diag << res.violating_variable_count << " fractional/invalid integer variable(s).";
        }
    }
    res.diagnostics = diag.str();

    return res;
}

MilpRelaxationResult MilpFoundation::solve_relaxation(
    const LPModel& model,
    real_t integrality_tolerance
) {
    MilpRelaxationResult res;
    auto t_start = std::chrono::high_resolution_clock::now();

    // Step 1: Classify
    auto t_c0 = std::chrono::high_resolution_clock::now();
    res.model_type = classify_model(model);
    auto t_c1 = std::chrono::high_resolution_clock::now();
    res.classification_time_ms = std::chrono::duration<double, std::milli>(t_c1 - t_c0).count();

    if (res.model_type == ModelType::UNSUPPORTED_MODEL) {
        res.status = MilpSolutionStatus::UNSUPPORTED_MODEL;
        res.status_message = "Model validation failed or contains invalid structure.";
        auto t_end = std::chrono::high_resolution_clock::now();
        res.total_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
        return res;
    }

    // Step 2: Extract Relaxation
    auto t_r0 = std::chrono::high_resolution_clock::now();
    LPRelaxation rel = extract_relaxation(model);
    res.relaxation_model = rel.relaxation_model;
    res.mapping = rel.mapping;
    auto t_r1 = std::chrono::high_resolution_clock::now();
    res.relaxation_time_ms = std::chrono::duration<double, std::milli>(t_r1 - t_r0).count();

    // Step 3: Solve LP Relaxation
    auto t_s0 = std::chrono::high_resolution_clock::now();
    DualRevisedSimplex dual_solver;
    DualRevisedSimplexResult lp_res = dual_solver.solve(res.relaxation_model);

    if (lp_res.status != DualRevisedSimplexStatus::OPTIMAL) {
        // Fallback to Revised Simplex
        RevisedSimplex primal_solver;
        RevisedSimplexResult primal_res = primal_solver.solve(res.relaxation_model);
        if (primal_res.status == RevisedSimplexStatus::OPTIMAL) {
            res.objective_value = primal_res.objective_value;
            res.primal_solution = primal_res.primal_solution;
            res.solver_used = "RevisedSimplex";
        } else {
            res.status = MilpSolutionStatus::NUMERICAL_FAILURE;
            res.status_message = "LP relaxation solver failed to converge.";
            auto t_end = std::chrono::high_resolution_clock::now();
            res.total_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
            return res;
        }
    } else {
        res.objective_value = lp_res.objective_value;
        res.primal_solution = lp_res.primal_solution;
        res.solver_used = "DualRevisedSimplex";
    }
    auto t_s1 = std::chrono::high_resolution_clock::now();
    res.solve_time_ms = std::chrono::duration<double, std::milli>(t_s1 - t_s0).count();

    // Step 4: Integer Feasibility Verification
    auto t_i0 = std::chrono::high_resolution_clock::now();
    res.integer_verification = verify_integer_feasibility(
        model, res.primal_solution, integrality_tolerance
    );
    auto t_i1 = std::chrono::high_resolution_clock::now();
    res.integer_check_time_ms = std::chrono::duration<double, std::milli>(t_i1 - t_i0).count();

    // Step 5: Classify Solution Result
    if (res.integer_verification.is_integer_feasible) {
        res.status = MilpSolutionStatus::INTEGER_FEASIBLE_SOLUTION;
        res.status_message = "LP relaxation solution is integer-feasible (note: LP relaxation solution, not global MILP proof).";
    } else {
        res.status = MilpSolutionStatus::LP_RELAXATION_OPTIMAL;
        res.status_message = "LP relaxation solved to optimality with fractional integer/binary variables.";
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    res.total_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    return res;
}

} // namespace bharatopt
