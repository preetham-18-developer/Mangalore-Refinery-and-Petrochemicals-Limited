#include <bharatopt/presolve.hpp>
#include <chrono>
#include <cmath>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace bharatopt {

// ============================================================================
// Postsolve Implementation
// ============================================================================

void Postsolve::initialize(const LPModel& orig_model) {
    orig_model_copy_ = orig_model;
    orig_num_vars_ = orig_model.num_variables();
    fixed_values_.assign(orig_num_vars_, 0.0);
    is_fixed_.assign(orig_num_vars_, false);
    orig_to_reduced_var_.assign(orig_num_vars_, -1);
    tightened_lb_.resize(orig_num_vars_);
    tightened_ub_.resize(orig_num_vars_);
    for (size_t i = 0; i < orig_num_vars_; ++i) {
        tightened_lb_[i] = orig_model.get_variable(static_cast<index_t>(i)).lower_bound;
        tightened_ub_[i] = orig_model.get_variable(static_cast<index_t>(i)).upper_bound;
    }
}

void Postsolve::record_fixed_var(index_t orig_var_idx, real_t val) {
    if (orig_var_idx >= 0 && static_cast<size_t>(orig_var_idx) < orig_num_vars_) {
        size_t idx = static_cast<size_t>(orig_var_idx);
        is_fixed_[idx] = true;
        fixed_values_[idx] = val;
        orig_to_reduced_var_[idx] = -1;
    }
}

void Postsolve::record_var_map(index_t orig_var_idx, index_t reduced_var_idx) {
    if (orig_var_idx >= 0 && static_cast<size_t>(orig_var_idx) < orig_num_vars_) {
        orig_to_reduced_var_[static_cast<size_t>(orig_var_idx)] = reduced_var_idx;
    }
}

void Postsolve::record_tightened_bounds(index_t orig_var_idx, real_t lb, real_t ub) {
    if (orig_var_idx >= 0 && static_cast<size_t>(orig_var_idx) < orig_num_vars_) {
        size_t idx = static_cast<size_t>(orig_var_idx);
        tightened_lb_[idx] = lb;
        tightened_ub_[idx] = ub;
    }
}

std::vector<real_t> Postsolve::recover_solution(const std::vector<real_t>& reduced_x) const {
    std::vector<real_t> orig_x(orig_num_vars_, 0.0);
    for (size_t i = 0; i < orig_num_vars_; ++i) {
        if (is_fixed_[i]) {
            orig_x[i] = fixed_values_[i];
        } else {
            index_t red_idx = orig_to_reduced_var_[i];
            if (red_idx >= 0 && static_cast<size_t>(red_idx) < reduced_x.size()) {
                orig_x[i] = reduced_x[static_cast<size_t>(red_idx)];
            } else {
                const auto& var = orig_model_copy_.get_variable(static_cast<index_t>(i));
                real_t t_lb = (i < tightened_lb_.size()) ? tightened_lb_[i] : var.lower_bound;
                real_t t_ub = (i < tightened_ub_.size()) ? tightened_ub_[i] : var.upper_bound;
                real_t c = (orig_model_copy_.sense() == ObjectiveSense::MINIMIZE) ? var.obj_coeff : -var.obj_coeff;
                if (c > 0.0) {
                    orig_x[i] = !std::isinf(t_lb) ? t_lb : (!std::isinf(t_ub) ? t_ub : 0.0);
                } else if (c < 0.0) {
                    orig_x[i] = !std::isinf(t_ub) ? t_ub : (!std::isinf(t_lb) ? t_lb : 0.0);
                } else {
                    if (!std::isinf(t_lb)) orig_x[i] = t_lb;
                    else if (!std::isinf(t_ub)) orig_x[i] = t_ub;
                    else orig_x[i] = 0.0;
                }
            }
        }
    }
    return orig_x;
}

bool Postsolve::verify_original_feasibility(const std::vector<real_t>& orig_x, real_t tol) const {
    if (orig_x.size() != orig_num_vars_) return false;

    // 1. Variable Bounds Check
    const auto& vars = orig_model_copy_.variables();
    for (size_t i = 0; i < vars.size(); ++i) {
        real_t val = orig_x[i];
        if (val < vars[i].lower_bound - tol || val > vars[i].upper_bound + tol) {
            return false;
        }
    }

    // 2. Constraint Residual Check
    const auto& constraints = orig_model_copy_.constraints();
    for (const auto& cons : constraints) {
        real_t lhs = 0.0;
        for (const auto& term : cons.terms) {
            if (term.first >= 0 && static_cast<size_t>(term.first) < orig_x.size()) {
                lhs += term.second * orig_x[static_cast<size_t>(term.first)];
            }
        }

        switch (cons.sense) {
            case ConstraintSense::LESS_EQUAL:
                if (lhs > cons.rhs + tol) return false;
                break;
            case ConstraintSense::GREATER_EQUAL:
                if (lhs < cons.rhs - tol) return false;
                break;
            case ConstraintSense::EQUAL:
                if (std::abs(lhs - cons.rhs) > tol) return false;
                break;
            case ConstraintSense::RANGED:
                if (lhs < cons.rhs - tol || lhs > cons.range_upper + tol) return false;
                break;
        }
    }

    return true;
}

real_t Postsolve::compute_original_objective(const std::vector<real_t>& orig_x) const {
    real_t obj = orig_model_copy_.obj_offset();
    const auto& vars = orig_model_copy_.variables();
    for (size_t i = 0; i < vars.size(); ++i) {
        if (i < orig_x.size()) {
            obj += vars[i].obj_coeff * orig_x[i];
        }
    }
    return obj;
}

// ============================================================================
// PresolveResult Implementation
// ============================================================================

std::string PresolveResult::to_string() const {
    std::ostringstream oss;
    oss << "Presolve Status: ";
    switch (status) {
        case PresolveStatus::SUCCESS: oss << "SUCCESS"; break;
        case PresolveStatus::INFEASIBLE: oss << "INFEASIBLE"; break;
        case PresolveStatus::UNBOUNDED: oss << "UNBOUNDED"; break;
        case PresolveStatus::NO_CHANGE: oss << "NO_CHANGE"; break;
    }
    oss << "\n";
    oss << "  Original Model: " << stats.original_vars << " Vars, " << stats.original_cons << " Cons\n";
    oss << "  Reduced Model:  " << stats.reduced_vars << " Vars, " << stats.reduced_cons << " Cons\n";
    oss << "  Reductions:     " << stats.vars_removed << " Vars Removed, " << stats.cons_removed << " Cons Removed\n";
    oss << "  Execution Time: " << stats.presolve_time_ms << " ms\n";
    return oss.str();
}

// ============================================================================
// PresolveEngine Implementation
// ============================================================================

PresolveResult PresolveEngine::presolve(const LPModel& orig_model, const PresolveOptions& options) const {
    auto start_time = std::chrono::high_resolution_clock::now();
    auto is_infinite_bound = [](real_t val) { return std::isinf(val) || std::abs(val) >= 1e19; };

    PresolveResult result;
    result.postsolve.initialize(orig_model);

    result.stats.original_vars = orig_model.num_variables();
    result.stats.original_cons = orig_model.num_constraints();

    // Working structures
    size_t num_vars = orig_model.num_variables();
    size_t num_cons = orig_model.num_constraints();

    std::vector<Variable> work_vars = orig_model.variables();
    std::vector<Constraint> work_cons = orig_model.constraints();
    real_t work_obj_offset = orig_model.obj_offset();
    ObjectiveSense sense = orig_model.sense();

    std::vector<bool> active_vars(num_vars, true);
    std::vector<bool> active_cons(num_cons, true);
    std::vector<real_t> fixed_vals(num_vars, 0.0);

    bool changed = true;
    size_t pass_count = 0;

    while (changed && pass_count < 100) {
        changed = false;
        pass_count++;

        // Pass A: Fixed Variable Elimination
        if (options.enable_fixed_variable) {
            for (size_t j = 0; j < num_vars; ++j) {
                if (!active_vars[j]) continue;

                real_t lb = work_vars[j].lower_bound;
                real_t ub = work_vars[j].upper_bound;

                if (lb > ub + 1e-12) {
                    result.status = PresolveStatus::INFEASIBLE;
                    auto end_time = std::chrono::high_resolution_clock::now();
                    result.stats.presolve_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
                    return result;
                }

                if (std::abs(lb - ub) <= 1e-12) {
                    real_t fix_val = lb;
                    active_vars[j] = false;
                    fixed_vals[j] = fix_val;
                    result.postsolve.record_fixed_var(static_cast<index_t>(j), fix_val);

                    // Update objective offset
                    work_obj_offset += work_vars[j].obj_coeff * fix_val;

                    // Substitute into constraints
                    for (size_t i = 0; i < num_cons; ++i) {
                        if (!active_cons[i]) continue;
                        auto& terms = work_cons[i].terms;
                        for (auto it = terms.begin(); it != terms.end(); ) {
                            if (static_cast<size_t>(it->first) == j) {
                                work_cons[i].rhs -= it->second * fix_val;
                                if (work_cons[i].sense == ConstraintSense::RANGED) {
                                    work_cons[i].range_upper -= it->second * fix_val;
                                }
                                it = terms.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }

                    changed = true;
                    result.stats.transformations.push_back({
                        ReductionType::FIXED_VARIABLE,
                        "Fixed variable " + work_vars[j].name + " at value " + std::to_string(fix_val),
                        work_vars[j].name, static_cast<index_t>(j), -1, fix_val
                    });
                }
            }
        }

        // Pass B: Empty Row Detection & Infeasibility
        if (options.enable_empty_row) {
            for (size_t i = 0; i < num_cons; ++i) {
                if (!active_cons[i]) continue;

                // Filter inactive terms
                auto& terms = work_cons[i].terms;
                terms.erase(std::remove_if(terms.begin(), terms.end(), [&](const std::pair<index_t, real_t>& t) {
                    return !active_vars[static_cast<size_t>(t.first)] || t.second == 0.0;
                }), terms.end());

                if (terms.empty()) {
                    real_t rhs = work_cons[i].rhs;
                    ConstraintSense csense = work_cons[i].sense;

                    bool infeasible = false;
                    if (csense == ConstraintSense::LESS_EQUAL && rhs < -1e-12) infeasible = true;
                    if (csense == ConstraintSense::GREATER_EQUAL && rhs > 1e-12) infeasible = true;
                    if (csense == ConstraintSense::EQUAL && std::abs(rhs) > 1e-12) infeasible = true;
                    if (csense == ConstraintSense::RANGED && (rhs > 1e-12 || work_cons[i].range_upper < -1e-12)) infeasible = true;

                    if (infeasible) {
                        result.status = PresolveStatus::INFEASIBLE;
                        auto end_time = std::chrono::high_resolution_clock::now();
                        result.stats.presolve_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
                        return result;
                    }

                    active_cons[i] = false;
                    changed = true;
                    result.stats.transformations.push_back({
                        ReductionType::EMPTY_ROW_REDUNDANT,
                        "Removed empty redundant constraint " + work_cons[i].name,
                        work_cons[i].name, -1, static_cast<index_t>(i), 0.0
                    });
                }
            }
        }

        // Pass C: Empty Column Processing
        if (options.enable_empty_col) {
            std::vector<size_t> col_counts(num_vars, 0);
            for (size_t i = 0; i < num_cons; ++i) {
                if (!active_cons[i]) continue;
                for (const auto& term : work_cons[i].terms) {
                    if (active_vars[static_cast<size_t>(term.first)]) {
                        col_counts[static_cast<size_t>(term.first)]++;
                    }
                }
            }

            for (size_t j = 0; j < num_vars; ++j) {
                if (!active_vars[j]) continue;
                if (col_counts[j] == 0) {
                    real_t cj = work_vars[j].obj_coeff;
                    real_t lb = work_vars[j].lower_bound;
                    real_t ub = work_vars[j].upper_bound;

                    real_t fix_val = 0.0;
                    bool fix_var = false;

                    if (cj == 0.0) {
                        fix_val = !is_infinite_bound(lb) ? lb : (!is_infinite_bound(ub) ? ub : 0.0);
                        fix_var = true;
                    } else if (sense == ObjectiveSense::MINIMIZE) {
                        if (cj > 0.0) {
                            if (!is_infinite_bound(lb)) { fix_val = lb; fix_var = true; }
                            else { result.status = PresolveStatus::UNBOUNDED; }
                        } else {
                            if (!is_infinite_bound(ub)) { fix_val = ub; fix_var = true; }
                            else { result.status = PresolveStatus::UNBOUNDED; }
                        }
                    } else { // MAXIMIZE
                        if (cj > 0.0) {
                            if (!is_infinite_bound(ub)) { fix_val = ub; fix_var = true; }
                            else { result.status = PresolveStatus::UNBOUNDED; }
                        } else {
                            if (!is_infinite_bound(lb)) { fix_val = lb; fix_var = true; }
                            else { result.status = PresolveStatus::UNBOUNDED; }
                        }
                    }

                    if (result.status == PresolveStatus::UNBOUNDED) {
                        auto end_time = std::chrono::high_resolution_clock::now();
                        result.stats.presolve_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
                        return result;
                    }

                    if (fix_var) {
                        active_vars[j] = false;
                        fixed_vals[j] = fix_val;
                        result.postsolve.record_fixed_var(static_cast<index_t>(j), fix_val);
                        work_obj_offset += cj * fix_val;
                        changed = true;
                        result.stats.transformations.push_back({
                            ReductionType::EMPTY_COL_FIXED,
                            "Fixed empty column " + work_vars[j].name + " at value " + std::to_string(fix_val),
                            work_vars[j].name, static_cast<index_t>(j), -1, fix_val
                        });
                    }
                }
            }
        }

        // Pass D: Singleton Row Processing & Bound Tightening
        if (options.enable_singleton_row) {
            for (size_t i = 0; i < num_cons; ++i) {
                if (!active_cons[i]) continue;
                const auto& terms = work_cons[i].terms;
                if (terms.size() == 1) {
                    index_t var_idx = terms[0].first;
                    size_t j = static_cast<size_t>(var_idx);
                    if (!active_vars[j]) continue;

                    real_t a = terms[0].second;
                    real_t b = work_cons[i].rhs;
                    ConstraintSense csense = work_cons[i].sense;

                    if (a != 0.0) {
                        real_t val = b / a;
                        real_t new_lb = -BHARATOPT_INFINITY;
                        real_t new_ub = BHARATOPT_INFINITY;

                        if (csense == ConstraintSense::LESS_EQUAL) {
                            if (a > 0.0) new_ub = val;
                            else new_lb = val;
                        } else if (csense == ConstraintSense::GREATER_EQUAL) {
                            if (a > 0.0) new_lb = val;
                            else new_ub = val;
                        } else if (csense == ConstraintSense::EQUAL) {
                            new_lb = val;
                            new_ub = val;
                        } else if (csense == ConstraintSense::RANGED) {
                            real_t u_val = work_cons[i].range_upper / a;
                            if (a > 0.0) {
                                new_lb = val;
                                new_ub = u_val;
                            } else {
                                new_lb = u_val;
                                new_ub = val;
                            }
                        }

                        // Tighten bounds
                        if (new_lb > work_vars[j].lower_bound) work_vars[j].lower_bound = new_lb;
                        if (new_ub < work_vars[j].upper_bound) work_vars[j].upper_bound = new_ub;
                        result.postsolve.record_tightened_bounds(static_cast<index_t>(j), work_vars[j].lower_bound, work_vars[j].upper_bound);

                        if (work_vars[j].lower_bound > work_vars[j].upper_bound + 1e-12) {
                            result.status = PresolveStatus::INFEASIBLE;
                            auto end_time = std::chrono::high_resolution_clock::now();
                            result.stats.presolve_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
                            return result;
                        }

                        active_cons[i] = false;
                        changed = true;
                        result.stats.transformations.push_back({
                            ReductionType::SINGLETON_ROW,
                            "Singleton row " + work_cons[i].name + " tightened variable " + work_vars[j].name,
                            work_vars[j].name, static_cast<index_t>(j), static_cast<index_t>(i), val
                        });
                    }
                }
            }
        }

        // Pass E: Big-M Coefficient Tightening
        if (options.enable_bound_tightening) {
            for (size_t i = 0; i < num_cons; ++i) {
                if (!active_cons[i]) continue;
                auto& terms = work_cons[i].terms;
                if (work_cons[i].sense != ConstraintSense::LESS_EQUAL) continue;

                for (auto& term : terms) {
                    index_t v_idx = term.first;
                    size_t j = static_cast<size_t>(v_idx);
                    if (!active_vars[j]) continue;

                    if (work_vars[j].type == VariableType::BINARY && term.second < -1e-5) {
                        real_t cur_coeff = term.second;

                        real_t max_pos_sum = 0.0;
                        bool can_bound = true;

                        for (const auto& o_term : terms) {
                            if (o_term.first == v_idx) continue;
                            size_t oj = static_cast<size_t>(o_term.first);
                            if (!active_vars[oj]) continue;

                            if (o_term.second > 0.0) {
                                if (std::isinf(work_vars[oj].upper_bound)) {
                                    can_bound = false;
                                    break;
                                }
                                max_pos_sum += o_term.second * work_vars[oj].upper_bound;
                            } else if (o_term.second < 0.0) {
                                if (!std::isinf(work_vars[oj].lower_bound)) {
                                    max_pos_sum += o_term.second * work_vars[oj].lower_bound;
                                }
                            }
                        }

                        if (can_bound && max_pos_sum > 0.0) {
                            real_t required_m = max_pos_sum - work_cons[i].rhs;
                            if (required_m > 0.0 && -cur_coeff > required_m + 1e-4) {
                                term.second = -required_m;
                                changed = true;
                                result.stats.transformations.push_back({
                                    ReductionType::BOUND_TIGHTENING,
                                    "Tightened Big-M coefficient on " + work_cons[i].name + " for " + work_vars[j].name + " from " + std::to_string(cur_coeff) + " to -" + std::to_string(required_m),
                                    work_vars[j].name, static_cast<index_t>(j), static_cast<index_t>(i), -required_m
                                });
                            }
                        }
                    }
                }
            }
        }
    }

    // Pass F: Build Reduced LPModel
    LPModel reduced("reduced_" + orig_model.name());
    reduced.set_sense(sense);
    reduced.set_obj_offset(work_obj_offset);

    std::vector<index_t> orig_to_red(num_vars, -1);
    for (size_t j = 0; j < num_vars; ++j) {
        if (active_vars[j]) {
            index_t red_idx = reduced.add_variable(
                work_vars[j].name,
                work_vars[j].lower_bound,
                work_vars[j].upper_bound,
                work_vars[j].obj_coeff,
                work_vars[j].type
            );
            orig_to_red[j] = red_idx;
            result.postsolve.record_var_map(static_cast<index_t>(j), red_idx);
        }
    }

    for (size_t i = 0; i < num_cons; ++i) {
        if (!active_cons[i]) continue;
        std::vector<std::pair<index_t, real_t>> red_terms;
        for (const auto& term : work_cons[i].terms) {
            index_t orig_v = term.first;
            if (orig_v >= 0 && static_cast<size_t>(orig_v) < num_vars && active_vars[static_cast<size_t>(orig_v)]) {
                red_terms.push_back({orig_to_red[static_cast<size_t>(orig_v)], term.second});
            }
        }
        if (!red_terms.empty()) {
            reduced.add_constraint(
                work_cons[i].name,
                red_terms,
                work_cons[i].sense,
                work_cons[i].rhs,
                work_cons[i].range_upper
            );
        }
    }

    result.reduced_model = reduced;
    result.stats.reduced_vars = reduced.num_variables();
    result.stats.reduced_cons = reduced.num_constraints();
    result.stats.vars_removed = result.stats.original_vars - result.stats.reduced_vars;
    result.stats.cons_removed = result.stats.original_cons - result.stats.reduced_cons;

    if (result.stats.vars_removed > 0 || result.stats.cons_removed > 0 || std::abs(work_obj_offset - orig_model.obj_offset()) > 1e-12) {
        result.status = PresolveStatus::SUCCESS;
    } else {
        result.status = PresolveStatus::NO_CHANGE;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    result.stats.presolve_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    return result;
}

} // namespace bharatopt
