#include <bharatopt/branch_and_bound.hpp>
#include <chrono>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <iostream>

namespace bharatopt {

std::string bnb_node_status_to_string(BnBNodeStatus status) {
    switch (status) {
        case BnBNodeStatus::OPEN: return "OPEN";
        case BnBNodeStatus::PROCESSING: return "PROCESSING";
        case BnBNodeStatus::INTEGER_FEASIBLE: return "INTEGER_FEASIBLE";
        case BnBNodeStatus::PRUNED_INFEASIBLE: return "PRUNED_INFEASIBLE";
        case BnBNodeStatus::PRUNED_BOUND: return "PRUNED_BOUND";
        case BnBNodeStatus::BRANCHED: return "BRANCHED";
        case BnBNodeStatus::NUMERICAL_FAILURE: return "NUMERICAL_FAILURE";
        case BnBNodeStatus::UNRESOLVED: return "UNRESOLVED";
        default: return "UNKNOWN";
    }
}

std::string bnb_solver_status_to_string(BnBSolverStatus status) {
    switch (status) {
        case BnBSolverStatus::OPTIMAL: return "OPTIMAL";
        case BnBSolverStatus::INFEASIBLE: return "INFEASIBLE";
        case BnBSolverStatus::LIMIT_REACHED: return "LIMIT_REACHED";
        case BnBSolverStatus::NUMERICAL_FAILURE: return "NUMERICAL_FAILURE";
        case BnBSolverStatus::UNRESOLVED: return "UNRESOLVED";
        case BnBSolverStatus::UNSUPPORTED_MODEL: return "UNSUPPORTED_MODEL";
        default: return "UNKNOWN";
    }
}

std::string BnBTelemetry::to_string() const {
    std::ostringstream oss;
    oss << "Branch-and-Bound Telemetry:\n"
        << "  Solver Status: " << bnb_solver_status_to_string(status) << "\n"
        << "  Total Solve Time: " << std::fixed << std::setprecision(2) << total_solve_time_ms << " ms\n"
        << "  Root Relaxation Time: " << root_relaxation_time_ms << " ms\n"
        << "  Nodes Created: " << nodes_created << "\n"
        << "  Nodes Processed: " << nodes_processed << "\n"
        << "  Nodes Pruned (Infeasibility): " << nodes_pruned_infeasibility << "\n"
        << "  Nodes Pruned (Bound): " << nodes_pruned_bound << "\n"
        << "  Integer-Feasible Nodes: " << integer_feasible_nodes << "\n"
        << "  Fractional Nodes: " << fractional_nodes << "\n"
        << "  Max Tree Depth: " << max_tree_depth << "\n"
        << "  Incumbent Updates: " << incumbent_updates << "\n"
        << "  Final Incumbent Obj: " << final_incumbent_obj << "\n"
        << "  Best Open Bound: " << best_open_bound << "\n"
        << "  Warm Start Mode: " << (warm_start_mode == WarmStartMode::WARM_START ? "WARM_START" : "COLD_START") << "\n"
        << "  Warm Starts Attempted: " << warm_starts_attempted << "\n"
        << "  Warm Starts Accepted: " << warm_starts_accepted << "\n"
        << "  Warm Starts Rejected: " << warm_starts_rejected << "\n"
        << "  Warm Starts Failed: " << warm_starts_failed << "\n"
        << "  Cold Fallbacks: " << cold_fallbacks;
    return oss.str();
}

BranchAndBoundEngine::BranchAndBoundEngine(BnBConfig config)
    : config_(config) {}

index_t BranchAndBoundEngine::select_branching_variable(
    const LPModel& model,
    const std::vector<real_t>& solution,
    real_t& out_val
) const {
    index_t best_var_idx = -1;
    real_t max_frac = -1.0;
    out_val = 0.0;

    const auto& vars = model.variables();
    for (size_t j = 0; j < vars.size(); ++j) {
        const auto& var = vars[j];
        if (var.type == VariableType::CONTINUOUS) {
            continue;
        }

        real_t val = solution[j];
        real_t floor_val = std::floor(val);
        real_t ceil_val = std::ceil(val);
        real_t frac = std::min(val - floor_val, ceil_val - val);

        if (frac > config_.integrality_tolerance) {
            // Select most fractional variable (largest distance to nearest integer)
            // Deterministic tie-breaking: lowest variable index j
            if (frac > max_frac + 1e-9) {
                max_frac = frac;
                best_var_idx = static_cast<index_t>(j);
                out_val = val;
            }
        }
    }

    return best_var_idx;
}

bool BranchAndBoundEngine::is_better_than_incumbent(
    real_t candidate_obj,
    const Incumbent& incumbent,
    ObjectiveSense sense
) const {
    if (!incumbent.has_incumbent) {
        return true;
    }
    if (sense == ObjectiveSense::MAXIMIZE) {
        return candidate_obj > incumbent.objective_value + config_.objective_tolerance;
    } else {
        return candidate_obj < incumbent.objective_value - config_.objective_tolerance;
    }
}

bool BranchAndBoundEngine::can_prune_by_bound(
    real_t node_bound,
    const Incumbent& incumbent,
    ObjectiveSense sense
) const {
    if (!incumbent.has_incumbent) {
        return false;
    }
    if (sense == ObjectiveSense::MAXIMIZE) {
        return node_bound <= incumbent.objective_value + config_.objective_tolerance;
    } else {
        return node_bound >= incumbent.objective_value - config_.objective_tolerance;
    }
}

struct LpSolveResult {
    bool is_optimal{false};
    bool is_infeasible{false};
    bool is_unbounded{false};
    real_t objective_value{0.0};
    std::vector<real_t> solution;
    Basis final_basis;
    WarmStartStatus warm_status{WarmStartStatus::COLD_START_USED};
};

static LpSolveResult solve_node_lp_relaxation(
    const LPModel& lp_model,
    const LPWarmStartState& parent_warm_state,
    WarmStartMode mode,
    BnBTelemetry& telemetry
) {
    LpSolveResult res;

    // Check if warm-start is enabled and parent basis is structurally valid
    if (mode == WarmStartMode::WARM_START && parent_warm_state.valid) {
        DualRevisedSimplex dual_solver;
        StandardFormLP std_lp = dual_solver.create_standard_form(lp_model);

        if (parent_warm_state.basis.num_rows == std_lp.num_rows &&
            parent_warm_state.basis.num_cols == std_lp.num_cols) {

            telemetry.warm_starts_attempted++;
            DualRevisedSimplexResult warm_res = dual_solver.solve_warm_start(lp_model, parent_warm_state.basis);

            if (warm_res.status == DualRevisedSimplexStatus::OPTIMAL) {
                bool bounds_ok = true;
                for (size_t j = 0; j < lp_model.num_variables(); ++j) {
                    const auto& v = lp_model.get_variable(static_cast<index_t>(j));
                    real_t val = warm_res.primal_solution[j];
                    if (val < v.lower_bound - 1e-4 || val > v.upper_bound + 1e-4) {
                        bounds_ok = false;
                        break;
                    }
                }

                if (bounds_ok) {
                    telemetry.warm_starts_accepted++;
                    res.is_optimal = true;
                    res.objective_value = warm_res.objective_value;
                    res.solution = warm_res.primal_solution;
                    res.final_basis = warm_res.final_basis;
                    res.warm_status = WarmStartStatus::WARM_START_USED;
                    return res;
                } else {
                    telemetry.warm_starts_failed++;
                    telemetry.cold_fallbacks++;
                }
            } else if (warm_res.status == DualRevisedSimplexStatus::UNSUPPORTED_INITIAL_BASIS) {
                telemetry.warm_starts_rejected++;
                telemetry.cold_fallbacks++;
            } else {
                telemetry.warm_starts_failed++;
                telemetry.cold_fallbacks++;
            }
        } else {
            telemetry.warm_starts_rejected++;
            telemetry.cold_fallbacks++;
        }
    }

    // Cold-Start Path: RevisedSimplex (Two-Phase Primal Simplex)
    res.warm_status = WarmStartStatus::COLD_START_USED;
    RevisedSimplex primal_solver;
    RevisedSimplexResult primal_res = primal_solver.solve(lp_model);

    if (primal_res.status == RevisedSimplexStatus::OPTIMAL) {
        res.is_optimal = true;
        res.objective_value = primal_res.objective_value;
        res.solution = primal_res.primal_solution;
        res.final_basis = primal_res.final_basis;
    } else if (primal_res.status == RevisedSimplexStatus::INFEASIBLE) {
        res.is_infeasible = true;
    } else if (primal_res.status == RevisedSimplexStatus::UNBOUNDED) {
        res.is_unbounded = true;
    }

    return res;
}

BnBResult BranchAndBoundEngine::solve(const LPModel& model) {
    BnBResult result;
    auto t_start = std::chrono::high_resolution_clock::now();
    result.telemetry.warm_start_mode = config_.warm_start_mode;

    // 1. Model Validation
    ModelValidator validator;
    ValidationResult val_res = validator.validate(model);
    if (!val_res.is_valid()) {
        result.status = BnBSolverStatus::UNSUPPORTED_MODEL;
        result.status_message = "Model validation failed before B&B execution.";
        result.telemetry.status = result.status;
        return result;
    }

    ModelType model_type = MilpFoundation::classify_model(model);
    if (model_type == ModelType::UNSUPPORTED_MODEL) {
        result.status = BnBSolverStatus::UNSUPPORTED_MODEL;
        result.status_message = "Model structure unsupported for MILP B&B.";
        result.telemetry.status = result.status;
        return result;
    }

    const auto& vars = model.variables();
    size_t num_vars = vars.size();

    // 2. Node Selection Comparator (BEST-BOUND)
    ObjectiveSense sense = model.sense();
    auto node_cmp = [sense](const BnBNode& a, const BnBNode& b) {
        if (sense == ObjectiveSense::MAXIMIZE) {
            if (std::abs(a.lp_obj_bound - b.lp_obj_bound) > 1e-9) {
                return a.lp_obj_bound < b.lp_obj_bound; // Larger bound first -> lower element in std::priority_queue
            }
        } else {
            if (std::abs(a.lp_obj_bound - b.lp_obj_bound) > 1e-9) {
                return a.lp_obj_bound > b.lp_obj_bound; // Smaller bound first
            }
        }
        // Tie-breaking: 1) smaller depth, 2) smaller node_id
        if (a.depth != b.depth) {
            return a.depth > b.depth;
        }
        return a.node_id > b.node_id;
    };

    std::priority_queue<BnBNode, std::vector<BnBNode>, decltype(node_cmp)> open_queue(node_cmp);

    // 3. Solve Root Node Relaxation
    auto t_r0 = std::chrono::high_resolution_clock::now();
    LPRelaxation root_rel = MilpFoundation::extract_relaxation(model);

    LPWarmStartState dummy_warm;
    LpSolveResult root_lp_res = solve_node_lp_relaxation(root_rel.relaxation_model, dummy_warm, config_.warm_start_mode, result.telemetry);
    auto t_r1 = std::chrono::high_resolution_clock::now();
    result.telemetry.root_relaxation_time_ms = std::chrono::duration<double, std::milli>(t_r1 - t_r0).count();

    if (root_lp_res.is_infeasible) {
        result.status = BnBSolverStatus::INFEASIBLE;
        result.status_message = "Root LP relaxation is infeasible; MILP is infeasible.";
        result.telemetry.status = result.status;
        auto t_end = std::chrono::high_resolution_clock::now();
        result.telemetry.total_solve_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
        return result;
    } else if (root_lp_res.is_unbounded) {
        result.status = BnBSolverStatus::UNRESOLVED;
        result.status_message = "Root LP relaxation is unbounded.";
        result.telemetry.status = result.status;
        auto t_end = std::chrono::high_resolution_clock::now();
        result.telemetry.total_solve_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
        return result;
    } else if (!root_lp_res.is_optimal) {
        result.status = BnBSolverStatus::NUMERICAL_FAILURE;
        result.status_message = "Root LP relaxation failed to solve.";
        result.telemetry.status = result.status;
        auto t_end = std::chrono::high_resolution_clock::now();
        result.telemetry.total_solve_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
        return result;
    }

    // Root node construction
    BnBNode root_node;
    root_node.node_id = 0;
    root_node.parent_node_id = -1;
    root_node.depth = 0;
    root_node.node_lower_bounds.resize(num_vars);
    root_node.node_upper_bounds.resize(num_vars);
    for (size_t j = 0; j < num_vars; ++j) {
        root_node.node_lower_bounds[j] = vars[j].lower_bound;
        root_node.node_upper_bounds[j] = vars[j].upper_bound;
    }
    root_node.lp_obj_bound = root_lp_res.objective_value;
    root_node.lp_solution = root_lp_res.solution;
    root_node.is_lp_optimal = true;
    root_node.status = BnBNodeStatus::OPEN;

    root_node.warm_start_state.valid = true;
    root_node.warm_start_state.num_rows = root_rel.relaxation_model.num_constraints();
    root_node.warm_start_state.num_cols = root_rel.relaxation_model.num_variables();
    root_node.warm_start_state.basis = root_lp_res.final_basis;
    root_node.warm_start_state.primal_solution = root_lp_res.solution;
    root_node.warm_start_state.lp_obj = root_lp_res.objective_value;

    result.telemetry.nodes_created = 1;
    result.telemetry.best_open_bound = root_node.lp_obj_bound;

    // Verify root integer feasibility
    IntegerVerificationResult root_integ_res = MilpFoundation::verify_integer_feasibility(
        model, root_node.lp_solution, config_.integrality_tolerance
    );

    if (root_integ_res.is_integer_feasible) {
        // Root LP relaxation is already integer-feasible!
        result.incumbent.has_incumbent = true;
        result.incumbent.objective_value = root_node.lp_obj_bound;
        result.incumbent.solution = root_node.lp_solution;
        result.incumbent.originating_node_id = 0;
        result.incumbent.verification = root_integ_res;

        result.status = BnBSolverStatus::OPTIMAL;
        result.objective_value = root_node.lp_obj_bound;
        result.solution = root_node.lp_solution;
        result.status_message = "Root LP relaxation solution is naturally integer-feasible.";

        result.telemetry.nodes_processed = 1;
        result.telemetry.integer_feasible_nodes = 1;
        result.telemetry.incumbent_updates = 1;
        result.telemetry.final_incumbent_obj = result.objective_value;
        result.telemetry.status = result.status;

        auto t_end = std::chrono::high_resolution_clock::now();
        result.telemetry.total_solve_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
        return result;
    }

    // Push root node to search queue
    open_queue.push(root_node);

    // 4. Branch-and-Bound Tree Search Loop
    index_t next_node_id = 1;

    while (!open_queue.empty()) {
        auto t_now = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(t_now - t_start).count();

        if (result.telemetry.nodes_processed >= config_.max_nodes || elapsed_ms >= config_.time_limit_ms) {
            result.status = BnBSolverStatus::LIMIT_REACHED;
            result.status_message = "Branch-and-Bound search limit reached.";
            break;
        }

        BnBNode curr_node = open_queue.top();
        open_queue.pop();

        result.telemetry.nodes_processed++;
        if (!open_queue.empty()) {
            result.telemetry.best_open_bound = open_queue.top().lp_obj_bound;
        }

        // Bound Pruning Check
        if (can_prune_by_bound(curr_node.lp_obj_bound, result.incumbent, sense)) {
            result.telemetry.nodes_pruned_bound++;
            continue;
        }

        // Integer Feasibility Check
        IntegerVerificationResult integ_res = MilpFoundation::verify_integer_feasibility(
            model, curr_node.lp_solution, config_.integrality_tolerance
        );

        if (integ_res.is_integer_feasible) {
            result.telemetry.integer_feasible_nodes++;
            if (is_better_than_incumbent(curr_node.lp_obj_bound, result.incumbent, sense)) {
                result.incumbent.has_incumbent = true;
                result.incumbent.objective_value = curr_node.lp_obj_bound;
                result.incumbent.solution = curr_node.lp_solution;
                result.incumbent.originating_node_id = curr_node.node_id;
                result.incumbent.verification = integ_res;

                result.telemetry.incumbent_updates++;
                result.telemetry.final_incumbent_obj = curr_node.lp_obj_bound;
            }
            continue;
        }

        // Branching Variable Selection
        real_t branch_val = 0.0;
        index_t branch_var_idx = select_branching_variable(model, curr_node.lp_solution, branch_val);

        if (branch_var_idx < 0) {
            // No fractional integer variable found, but integer check returned false (e.g. bound/residual violation)
            continue;
        }

        result.telemetry.fractional_nodes++;

        // --- LEFT CHILD: x_j <= floor(val) ---
        real_t left_ub = std::floor(branch_val);
        real_t curr_left_lb = curr_node.node_lower_bounds[branch_var_idx];
        real_t curr_left_ub = curr_node.node_upper_bounds[branch_var_idx];
        real_t new_left_ub = std::min(curr_left_ub, left_ub);

        if (curr_left_lb <= new_left_ub + 1e-9) {
            BnBNode left_node;
            left_node.node_id = next_node_id++;
            left_node.parent_node_id = curr_node.node_id;
            left_node.depth = curr_node.depth + 1;
            left_node.node_lower_bounds = curr_node.node_lower_bounds;
            left_node.node_upper_bounds = curr_node.node_upper_bounds;
            left_node.node_upper_bounds[branch_var_idx] = new_left_ub;

            result.telemetry.max_tree_depth = std::max(result.telemetry.max_tree_depth, left_node.depth);

            // Solve Left Child LP Relaxation
            LPRelaxation left_rel = MilpFoundation::extract_relaxation(model);
            for (size_t j = 0; j < num_vars; ++j) {
                auto& v = left_rel.relaxation_model.get_variable(static_cast<index_t>(j));
                v.lower_bound = left_node.node_lower_bounds[j];
                v.upper_bound = left_node.node_upper_bounds[j];
            }

            LpSolveResult left_lp_res = solve_node_lp_relaxation(
                left_rel.relaxation_model,
                curr_node.warm_start_state,
                config_.warm_start_mode,
                result.telemetry
            );

            if (left_lp_res.is_optimal) {
                left_node.lp_obj_bound = left_lp_res.objective_value;
                left_node.lp_solution = left_lp_res.solution;
                left_node.is_lp_optimal = true;
                left_node.status = BnBNodeStatus::OPEN;

                left_node.warm_start_state.valid = true;
                left_node.warm_start_state.num_rows = left_rel.relaxation_model.num_constraints();
                left_node.warm_start_state.num_cols = left_rel.relaxation_model.num_variables();
                left_node.warm_start_state.basis = left_lp_res.final_basis;
                left_node.warm_start_state.primal_solution = left_lp_res.solution;
                left_node.warm_start_state.lp_obj = left_lp_res.objective_value;

                result.telemetry.nodes_created++;
                if (!can_prune_by_bound(left_node.lp_obj_bound, result.incumbent, sense)) {
                    open_queue.push(left_node);
                } else {
                    result.telemetry.nodes_pruned_bound++;
                }
            } else if (left_lp_res.is_infeasible) {
                result.telemetry.nodes_pruned_infeasibility++;
            }
        } else {
            // Contradictory bounds
            result.telemetry.nodes_pruned_infeasibility++;
        }

        // --- RIGHT CHILD: x_j >= ceil(val) ---
        real_t right_lb = std::ceil(branch_val);
        real_t curr_right_lb = curr_node.node_lower_bounds[branch_var_idx];
        real_t curr_right_ub = curr_node.node_upper_bounds[branch_var_idx];
        real_t new_right_lb = std::max(curr_right_lb, right_lb);

        if (new_right_lb <= curr_right_ub + 1e-9) {
            BnBNode right_node;
            right_node.node_id = next_node_id++;
            right_node.parent_node_id = curr_node.node_id;
            right_node.depth = curr_node.depth + 1;
            right_node.node_lower_bounds = curr_node.node_lower_bounds;
            right_node.node_upper_bounds = curr_node.node_upper_bounds;
            right_node.node_lower_bounds[branch_var_idx] = new_right_lb;

            result.telemetry.max_tree_depth = std::max(result.telemetry.max_tree_depth, right_node.depth);

            // Solve Right Child LP Relaxation
            LPRelaxation right_rel = MilpFoundation::extract_relaxation(model);
            for (size_t j = 0; j < num_vars; ++j) {
                auto& v = right_rel.relaxation_model.get_variable(static_cast<index_t>(j));
                v.lower_bound = right_node.node_lower_bounds[j];
                v.upper_bound = right_node.node_upper_bounds[j];
            }

            LpSolveResult right_lp_res = solve_node_lp_relaxation(
                right_rel.relaxation_model,
                curr_node.warm_start_state,
                config_.warm_start_mode,
                result.telemetry
            );

            if (right_lp_res.is_optimal) {
                right_node.lp_obj_bound = right_lp_res.objective_value;
                right_node.lp_solution = right_lp_res.solution;
                right_node.is_lp_optimal = true;
                right_node.status = BnBNodeStatus::OPEN;

                right_node.warm_start_state.valid = true;
                right_node.warm_start_state.num_rows = right_rel.relaxation_model.num_constraints();
                right_node.warm_start_state.num_cols = right_rel.relaxation_model.num_variables();
                right_node.warm_start_state.basis = right_lp_res.final_basis;
                right_node.warm_start_state.primal_solution = right_lp_res.solution;
                right_node.warm_start_state.lp_obj = right_lp_res.objective_value;

                result.telemetry.nodes_created++;
                if (!can_prune_by_bound(right_node.lp_obj_bound, result.incumbent, sense)) {
                    open_queue.push(right_node);
                } else {
                    result.telemetry.nodes_pruned_bound++;
                }
            } else if (right_lp_res.is_infeasible) {
                result.telemetry.nodes_pruned_infeasibility++;
            }
        } else {
            // Contradictory bounds
            result.telemetry.nodes_pruned_infeasibility++;
        }
    }

    // 5. Final Result Synthesis & Verification
    if (result.status != BnBSolverStatus::LIMIT_REACHED) {
        if (result.incumbent.has_incumbent) {
            // Independently verify incumbent solution
            IntegerVerificationResult final_verify = MilpFoundation::verify_integer_feasibility(
                model, result.incumbent.solution, config_.integrality_tolerance
            );

            if (final_verify.is_integer_feasible) {
                result.status = BnBSolverStatus::OPTIMAL;
                result.solution = result.incumbent.solution;

                // Recompute objective independently: c^T x + obj_offset
                real_t calc_obj = model.obj_offset();
                for (size_t j = 0; j < num_vars; ++j) {
                    calc_obj += vars[j].obj_coeff * result.solution[j];
                }
                result.objective_value = calc_obj;
                result.status_message = "Branch-and-Bound search completed; verified optimal MILP solution found.";
            } else {
                result.status = BnBSolverStatus::NUMERICAL_FAILURE;
                result.status_message = "Incumbent solution failed independent integer verification.";
            }
        } else {
            result.status = BnBSolverStatus::INFEASIBLE;
            result.status_message = "Exhaustive Branch-and-Bound search proved MILP infeasibility.";
        }
    } else {
        if (result.incumbent.has_incumbent) {
            result.solution = result.incumbent.solution;
            real_t calc_obj = model.obj_offset();
            for (size_t j = 0; j < num_vars; ++j) {
                calc_obj += vars[j].obj_coeff * result.solution[j];
            }
            result.objective_value = calc_obj;
        }
    }

    result.telemetry.status = result.status;
    auto t_end = std::chrono::high_resolution_clock::now();
    result.telemetry.total_solve_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    return result;
}

} // namespace bharatopt
