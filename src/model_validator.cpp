#include <bharatopt/model_validator.hpp>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace bharatopt {

std::string ValidationIssue::to_string() const {
    std::ostringstream oss;
    oss << "[" << (severity == IssueSeverity::ERROR ? "ERROR" : "WARNING") << "] "
        << "[" << category << "] ";
    if (!entity_name.empty()) {
        oss << "Entity '" << entity_name << "'";
        if (entity_index >= 0) {
            oss << " (#" << entity_index << ")";
        }
        oss << ": ";
    } else if (entity_index >= 0) {
        oss << "Entity #" << entity_index << ": ";
    }
    oss << message;
    return oss.str();
}

void ValidationResult::add_issue(IssueSeverity severity,
                                 const std::string& category,
                                 const std::string& message,
                                 const std::string& entity_name,
                                 index_t entity_index) {
    ValidationIssue issue{severity, category, message, entity_name, entity_index};
    issues_.push_back(issue);
    if (severity == IssueSeverity::ERROR) {
        error_count_++;
    } else {
        warning_count_++;
    }
}

std::vector<ValidationIssue> ValidationResult::errors() const {
    std::vector<ValidationIssue> errs;
    for (const auto& issue : issues_) {
        if (issue.severity == IssueSeverity::ERROR) {
            errs.push_back(issue);
        }
    }
    return errs;
}

std::vector<ValidationIssue> ValidationResult::warnings() const {
    std::vector<ValidationIssue> warns;
    for (const auto& issue : issues_) {
        if (issue.severity == IssueSeverity::WARNING) {
            warns.push_back(issue);
        }
    }
    return warns;
}

std::string ValidationResult::to_string() const {
    std::ostringstream oss;
    oss << "Validation Status: " << (is_valid() ? "VALID" : "INVALID")
        << " (" << error_count_ << " errors, " << warning_count_ << " warnings)\n";
    for (const auto& issue : issues_) {
        oss << "  " << issue.to_string() << "\n";
    }
    return oss.str();
}

ValidationResult ModelValidator::validate(const LPModel& model) const {
    ValidationResult result;

    // 1. Structure Validation
    if (model.name().empty()) {
        result.add_warning("STRUCTURE", "Model name is empty.");
    }

    // 2. Objective Validation
    if (std::isnan(model.obj_offset())) {
        result.add_error("OBJECTIVE", "Objective constant offset is NaN.");
    } else if (std::isinf(model.obj_offset())) {
        result.add_error("OBJECTIVE", "Objective constant offset is infinite.");
    }

    // 3. Variable Validation
    std::unordered_set<std::string> seen_var_names;
    const auto& vars = model.variables();

    for (size_t i = 0; i < vars.size(); ++i) {
        const auto& var = vars[i];
        index_t idx = static_cast<index_t>(i);

        // Name check
        if (seen_var_names.count(var.name) > 0) {
            result.add_error("VARIABLE", "Duplicate variable name '" + var.name + "'.", var.name, idx);
        }
        seen_var_names.insert(var.name);

        // NaN bound checks
        if (std::isnan(var.lower_bound)) {
            result.add_error("VARIABLE", "Variable lower bound is NaN.", var.name, idx);
        }
        if (std::isnan(var.upper_bound)) {
            result.add_error("VARIABLE", "Variable upper bound is NaN.", var.name, idx);
        }

        // Bound consistency check
        if (!std::isnan(var.lower_bound) && !std::isnan(var.upper_bound)) {
            if (var.lower_bound > var.upper_bound) {
                result.add_error("VARIABLE",
                                 "Lower bound (" + std::to_string(var.lower_bound) +
                                 ") > Upper bound (" + std::to_string(var.upper_bound) + ").",
                                 var.name, idx);
            }
        }

        // Objective coefficient check
        if (std::isnan(var.obj_coeff)) {
            result.add_error("OBJECTIVE", "Objective coefficient is NaN.", var.name, idx);
        } else if (std::isinf(var.obj_coeff)) {
            result.add_error("OBJECTIVE", "Objective coefficient is infinite.", var.name, idx);
        }

        // Type-specific checks
        if (var.type == VariableType::BINARY) {
            if (var.lower_bound < 0.0 || var.upper_bound > 1.0) {
                result.add_error("VARIABLE", "Binary variable bounds must be within [0, 1].", var.name, idx);
            } else if (var.lower_bound != 0.0 || var.upper_bound != 1.0) {
                result.add_warning("VARIABLE", "Binary variable has non-standard restricted bounds.", var.name, idx);
            }
        } else if (var.type == VariableType::INTEGER) {
            if (!std::isinf(var.lower_bound) && var.lower_bound != std::floor(var.lower_bound)) {
                result.add_warning("VARIABLE", "Integer variable has non-integer lower bound.", var.name, idx);
            }
            if (!std::isinf(var.upper_bound) && var.upper_bound != std::ceil(var.upper_bound)) {
                result.add_warning("VARIABLE", "Integer variable has non-integer upper bound.", var.name, idx);
            }
        }
    }

    // 4. Constraint Validation
    std::unordered_set<std::string> seen_cons_names;
    const auto& constraints = model.constraints();

    for (size_t i = 0; i < constraints.size(); ++i) {
        const auto& cons = constraints[i];
        index_t idx = static_cast<index_t>(i);

        // Name check
        if (seen_cons_names.count(cons.name) > 0) {
            result.add_error("CONSTRAINT", "Duplicate constraint name '" + cons.name + "'.", cons.name, idx);
        }
        seen_cons_names.insert(cons.name);

        // RHS NaN/Inf checks
        if (std::isnan(cons.rhs)) {
            result.add_error("CONSTRAINT", "Constraint RHS is NaN.", cons.name, idx);
        } else if (std::isinf(cons.rhs)) {
            result.add_error("CONSTRAINT", "Constraint RHS is infinite.", cons.name, idx);
        }

        // Ranged constraint check
        if (cons.sense == ConstraintSense::RANGED) {
            if (std::isnan(cons.range_upper)) {
                result.add_error("CONSTRAINT", "Ranged constraint upper bound is NaN.", cons.name, idx);
            } else if (std::isinf(cons.range_upper)) {
                result.add_error("CONSTRAINT", "Ranged constraint upper bound is infinite.", cons.name, idx);
            } else if (!std::isnan(cons.rhs) && cons.rhs > cons.range_upper) {
                result.add_error("CONSTRAINT",
                                 "Ranged constraint lower bound (RHS = " + std::to_string(cons.rhs) +
                                 ") > upper range (" + std::to_string(cons.range_upper) + ").",
                                 cons.name, idx);
            }
        }

        // Terms validation
        std::unordered_set<index_t> seen_term_vars;
        for (const auto& term : cons.terms) {
            index_t var_idx = term.first;
            real_t coeff = term.second;

            // Out-of-bounds variable reference
            if (var_idx < 0 || static_cast<size_t>(var_idx) >= vars.size()) {
                result.add_error("COEFFICIENT", "Constraint references invalid variable index #" + std::to_string(var_idx) + ".", cons.name, idx);
                continue;
            }

            // Coefficient checks
            if (std::isnan(coeff)) {
                result.add_error("COEFFICIENT", "Constraint term coefficient is NaN.", cons.name, idx);
            } else if (std::isinf(coeff)) {
                result.add_error("COEFFICIENT", "Constraint term coefficient is infinite.", cons.name, idx);
            } else if (coeff == 0.0) {
                result.add_warning("COEFFICIENT", "Constraint contains explicit zero coefficient.", cons.name, idx);
            }

            // Duplicate terms check
            if (seen_term_vars.count(var_idx) > 0) {
                result.add_warning("COEFFICIENT", "Constraint contains duplicate terms for variable #" + std::to_string(var_idx) + ".", cons.name, idx);
            }
            seen_term_vars.insert(var_idx);
        }
    }

    return result;
}

std::string InfeasibilityDiagnosis::to_string() const {
    if (!is_infeasible) return "No infeasibility conflicts detected.";
    std::ostringstream oss;
    oss << "Constraint Conflict Detected: ";
    if (!conflicting_rows.empty()) {
        for (size_t i = 0; i < conflicting_rows.size(); ++i) {
            if (i + 1 == conflicting_rows.size()) {
                oss << " vs " << conflicting_rows[i];
            } else {
                oss << conflicting_rows[i] << (i + 2 < conflicting_rows.size() ? " ∩ " : "");
            }
        }
    }
    if (!detailed_analysis.empty()) {
        oss << ", " << detailed_analysis;
    }
    return oss.str();
}

static std::string format_number(real_t val) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << val;
    std::string str = oss.str();
    if (str.find('.') != std::string::npos) {
        while (str.back() == '0') str.pop_back();
        if (str.back() == '.') str.pop_back();
    }
    return str;
}

InfeasibilityDiagnosis InfeasibilityAnalyzer::analyze(const LPModel& model) {
    InfeasibilityDiagnosis diag;
    const auto& vars = model.variables();
    const auto& constraints = model.constraints();

    // 1. Variable bounds check
    for (const auto& var : vars) {
        if (var.lower_bound > var.upper_bound + 1e-9) {
            diag.is_infeasible = true;
            diag.summary = "Variable bound contradiction";
            diag.conflicting_rows = {var.name};
            diag.detailed_analysis = "forced min " + format_number(var.lower_bound) + " > required " + format_number(var.upper_bound);
            return diag;
        }
    }

    // 2. Per-row implied bounds analysis
    struct RowBounds {
        std::string name;
        real_t min_lhs{0.0};
        real_t max_lhs{0.0};
        real_t req_lower{-BHARATOPT_INFINITY};
        real_t req_upper{BHARATOPT_INFINITY};
        ConstraintSense sense;
    };

    std::vector<RowBounds> row_info;
    row_info.reserve(constraints.size());

    for (const auto& cons : constraints) {
        real_t min_lhs = 0.0;
        real_t max_lhs = 0.0;
        bool min_inf = false;
        bool max_inf = false;

        for (const auto& term : cons.terms) {
            if (term.first < 0 || static_cast<size_t>(term.first) >= vars.size()) continue;
            const auto& v = vars[static_cast<size_t>(term.first)];
            real_t a = term.second;

            if (a > 0.0) {
                if (std::isinf(v.lower_bound)) min_inf = true;
                else min_lhs += a * v.lower_bound;

                if (std::isinf(v.upper_bound)) max_inf = true;
                else max_lhs += a * v.upper_bound;
            } else if (a < 0.0) {
                if (std::isinf(v.upper_bound)) min_inf = true;
                else min_lhs += a * v.upper_bound;

                if (std::isinf(v.lower_bound)) max_inf = true;
                else max_lhs += a * v.lower_bound;
            }
        }

        real_t req_lower = -BHARATOPT_INFINITY;
        real_t req_upper = BHARATOPT_INFINITY;

        if (cons.sense == ConstraintSense::LESS_EQUAL) {
            req_upper = cons.rhs;
        } else if (cons.sense == ConstraintSense::GREATER_EQUAL) {
            req_lower = cons.rhs;
        } else if (cons.sense == ConstraintSense::EQUAL) {
            req_lower = cons.rhs;
            req_upper = cons.rhs;
        } else if (cons.sense == ConstraintSense::RANGED) {
            req_lower = cons.rhs;
            req_upper = cons.range_upper;
        }

        RowBounds rb;
        rb.name = cons.name;
        rb.min_lhs = min_inf ? -BHARATOPT_INFINITY : min_lhs;
        rb.max_lhs = max_inf ? BHARATOPT_INFINITY : max_lhs;
        rb.req_lower = req_lower;
        rb.req_upper = req_upper;
        rb.sense = cons.sense;
        row_info.push_back(rb);

        // Check single-row impossibility
        if (!min_inf && min_lhs > req_upper + 1e-9) {
            diag.is_infeasible = true;
            diag.summary = "Single row min LHS exceeds upper bound";
            diag.conflicting_rows = {cons.name};
            diag.detailed_analysis = "forced min " + format_number(min_lhs) + " > required " + format_number(req_upper);
            return diag;
        }
        if (!max_inf && max_lhs < req_lower - 1e-9) {
            diag.is_infeasible = true;
            diag.summary = "Single row max LHS below lower bound";
            diag.conflicting_rows = {cons.name};
            diag.detailed_analysis = "forced max " + format_number(max_lhs) + " < required " + format_number(req_lower);
            return diag;
        }
    }

    // 3. Multi-row Overlap Analysis (Group RANGES / LO vs Target Equality/Capacity Row)
    for (size_t target_idx = 0; target_idx < constraints.size(); ++target_idx) {
        const auto& target_cons = constraints[target_idx];
        const auto& target_rb = row_info[target_idx];
        if (target_rb.req_upper == BHARATOPT_INFINITY) continue;

        // Map variable index to target coefficient c_j (for c_j > 0)
        std::unordered_map<index_t, real_t> target_coeffs;
        for (const auto& term : target_cons.terms) {
            if (term.first >= 0 && static_cast<size_t>(term.first) < vars.size() && term.second > 0.0) {
                target_coeffs[term.first] = term.second;
            }
        }
        if (target_coeffs.empty()) continue;

        // Individual variable lower bounds (updated by singleton rows)
        std::unordered_map<index_t, real_t> eff_var_lb;
        std::unordered_map<index_t, std::string> eff_var_lb_source;
        for (const auto& kv : target_coeffs) {
            index_t v_idx = kv.first;
            real_t lb = vars[static_cast<size_t>(v_idx)].lower_bound;
            eff_var_lb[v_idx] = (std::isinf(lb) || lb < 0.0) ? 0.0 : lb;
        }

        // Identify candidate sub-constraints S that provide a lower bound on a subset of target variables
        struct SubGroup {
            std::string name;
            real_t forced_min_contribution{0.0};
            std::unordered_set<index_t> var_set;
        };
        std::vector<SubGroup> candidate_groups;

        for (size_t sub_idx = 0; sub_idx < constraints.size(); ++sub_idx) {
            if (sub_idx == target_idx) continue;
            const auto& sub_cons = constraints[sub_idx];

            real_t sub_bound = -BHARATOPT_INFINITY;
            if (sub_cons.sense == ConstraintSense::GREATER_EQUAL || sub_cons.sense == ConstraintSense::EQUAL) {
                sub_bound = sub_cons.rhs;
            } else if (sub_cons.sense == ConstraintSense::RANGED) {
                sub_bound = sub_cons.rhs; // For ranged row (L-type or E-type), rhs is lower bound
            }

            if (sub_bound <= 0.0) continue;

            // Check if all variables in sub_cons belong to target_coeffs and have matching scale s_S
            bool valid = true;
            real_t scale = -1.0;
            std::unordered_set<index_t> sub_vars;

            for (const auto& sterm : sub_cons.terms) {
                index_t sv = sterm.first;
                real_t sa = sterm.second;
                if (sa <= 0.0 || target_coeffs.find(sv) == target_coeffs.end()) {
                    valid = false;
                    break;
                }
                real_t tc = target_coeffs[sv];
                real_t r = sa / tc;
                if (scale < 0.0) {
                    scale = r;
                } else if (std::abs(r - scale) > 1e-6) {
                    valid = false;
                    break;
                }
                sub_vars.insert(sv);
            }

            if (!valid || scale <= 0.0 || sub_vars.empty()) continue;

            real_t contribution = sub_bound / scale;

            if (sub_vars.size() == 1) {
                // Singleton constraint: update individual lower bound
                index_t sv = *sub_vars.begin();
                if (contribution > eff_var_lb[sv]) {
                    eff_var_lb[sv] = contribution;
                    eff_var_lb_source[sv] = sub_cons.name;
                }
            } else {
                // Group constraint
                candidate_groups.push_back({sub_cons.name, contribution, sub_vars});
            }
        }

        // Greedy selection of mutually disjoint group constraints that maximize forced lower bound contribution
        std::vector<std::string> contributor_rows;
        std::unordered_set<index_t> covered_vars;
        real_t total_group_min = 0.0;

        for (const auto& grp : candidate_groups) {
            bool overlaps = false;
            for (index_t v : grp.var_set) {
                if (covered_vars.count(v) > 0) {
                    overlaps = true;
                    break;
                }
            }
            if (!overlaps) {
                total_group_min += grp.forced_min_contribution;
                contributor_rows.push_back(grp.name);
                for (index_t v : grp.var_set) {
                    covered_vars.insert(v);
                }
            }
        }

        // Add contribution from uncovered variables using their individual lower bounds
        real_t total_indiv_min = 0.0;
        for (const auto& kv : target_coeffs) {
            index_t v_idx = kv.first;
            real_t c_j = kv.second;
            if (covered_vars.count(v_idx) == 0) {
                total_indiv_min += c_j * eff_var_lb[v_idx];
                if (eff_var_lb_source.count(v_idx) > 0) {
                    contributor_rows.push_back(eff_var_lb_source[v_idx]);
                }
            }
        }

        // Calculate negative terms contribution for target constraint
        real_t neg_contribution = 0.0;
        bool neg_unbounded = false;

        for (const auto& term : target_cons.terms) {
            if (term.first >= 0 && static_cast<size_t>(term.first) < vars.size() && term.second < 0.0) {
                const auto& v = vars[static_cast<size_t>(term.first)];
                if (std::isinf(v.upper_bound) || v.upper_bound >= BHARATOPT_INFINITY / 2.0) {
                    neg_unbounded = true;
                    break;
                } else {
                    neg_contribution += term.second * v.upper_bound;
                }
            }
        }

        if (neg_unbounded) continue;

        real_t total_forced_min = total_group_min + total_indiv_min + neg_contribution;

        if (total_forced_min > target_rb.req_upper + 1e-9) {
            diag.is_infeasible = true;
            diag.summary = "Multi-row interval overlap contradiction";
            contributor_rows.push_back(target_cons.name);
            diag.conflicting_rows = contributor_rows;
            diag.detailed_analysis = "forced min " + format_number(total_forced_min) + " > required " + format_number(target_rb.req_upper);
            return diag;
        }
    }

    return diag;
}

} // namespace bharatopt
