#include <bharatopt/model_validator.hpp>
#include <cmath>
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

} // namespace bharatopt
