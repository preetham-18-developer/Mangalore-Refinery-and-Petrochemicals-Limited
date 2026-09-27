#include <bharatopt/solution_verifier.hpp>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace bharatopt {

std::string VerificationResult::to_string() const {
    std::ostringstream oss;
    oss << "Verification Result:\n"
        << "  Status: " << (verified ? "VERIFIED" : "VERIFICATION_FAILED") << "\n"
        << "  Message: " << status_message << "\n"
        << "  Variables Evaluated: " << variable_count << "\n"
        << "  Constraints Evaluated: " << constraint_count << "\n"
        << "  Recomputed Objective: " << std::fixed << std::setprecision(6) << objective_recomputed << "\n";
    
    if (has_reported_objective) {
        oss << "  Reported Objective: " << objective_reported << "\n"
            << "  Objective Difference: " << objective_difference << "\n";
    }

    oss << "  Violations:\n"
        << "    Max Lower Bound Violation: " << maximum_lower_bound_violation << " (Count: " << violated_bound_count << ")\n"
        << "    Max Upper Bound Violation: " << maximum_upper_bound_violation << "\n"
        << "    Max Constraint Violation:  " << maximum_constraint_violation << " (Count: " << violated_constraint_count << ")\n"
        << "    Max Integrality Violation: " << maximum_integrality_violation << " (Count: " << violated_integrality_count << ")\n";

    if (!diagnostic_messages.empty()) {
        oss << "  Diagnostics:\n";
        for (const auto& msg : diagnostic_messages) {
            oss << "    - " << msg << "\n";
        }
    }

    return oss.str();
}

SolutionVerifier::SolutionVerifier(VerificationOptions options)
    : default_options_(options) {}

VerificationResult SolutionVerifier::verify(const LPModel& model,
                                            const std::vector<real_t>& candidate_solution,
                                            const VerificationOptions& options) const {
    VerificationResult res;
    res.variable_count = model.num_variables();
    res.constraint_count = model.num_constraints();
    res.has_reported_objective = false;
    res.verified = true;

    // 1. Dimension Check
    if (candidate_solution.size() != res.variable_count) {
        res.verified = false;
        res.status_message = "Dimension mismatch between model variables and candidate solution.";
        std::ostringstream oss;
        oss << "Candidate size (" << candidate_solution.size() << ") != Model variable count (" << res.variable_count << ")";
        res.diagnostic_messages.push_back(oss.str());
        return res;
    }

    // 2. Variable Validation (NaN / Inf and Bounds)
    for (size_t j = 0; j < res.variable_count; ++j) {
        const auto& var = model.get_variable(static_cast<index_t>(j));
        real_t val = candidate_solution[j];

        if (std::isnan(val) || std::isinf(val)) {
            res.verified = false;
            res.violated_bound_count++;
            res.maximum_lower_bound_violation = BHARATOPT_INFINITY;
            res.maximum_upper_bound_violation = BHARATOPT_INFINITY;
            std::ostringstream oss;
            oss << "Variable '" << var.name << "' (index " << j << ") is non-finite (NaN/Inf).";
            res.diagnostic_messages.push_back(oss.str());
            continue;
        }

        // Lower bound check
        if (val < var.lower_bound - options.feasibility_tolerance) {
            real_t viol = var.lower_bound - val;
            res.maximum_lower_bound_violation = std::max(res.maximum_lower_bound_violation, viol);
            res.violated_bound_count++;
            res.verified = false;
            std::ostringstream oss;
            oss << "Variable '" << var.name << "' violates lower bound: " << val << " < " << var.lower_bound;
            res.diagnostic_messages.push_back(oss.str());
        }

        // Upper bound check
        if (val > var.upper_bound + options.feasibility_tolerance) {
            real_t viol = val - var.upper_bound;
            res.maximum_upper_bound_violation = std::max(res.maximum_upper_bound_violation, viol);
            res.violated_bound_count++;
            res.verified = false;
            std::ostringstream oss;
            oss << "Variable '" << var.name << "' violates upper bound: " << val << " > " << var.upper_bound;
            res.diagnostic_messages.push_back(oss.str());
        }

        // Integrality check
        if (var.type != VariableType::CONTINUOUS) {
            real_t rounded = std::round(val);
            real_t integ_viol = std::abs(val - rounded);
            res.maximum_integrality_violation = std::max(res.maximum_integrality_violation, integ_viol);

            if (integ_viol > options.integrality_tolerance) {
                res.violated_integrality_count++;
                res.verified = false;
                std::ostringstream oss;
                oss << "Variable '" << var.name << "' (type: " 
                    << (var.type == VariableType::BINARY ? "BINARY" : "INTEGER")
                    << ") violates integrality: value=" << val << ", dist_to_integer=" << integ_viol;
                res.diagnostic_messages.push_back(oss.str());
            }
        }
    }

    // 3. Constraint Activity Recomputation
    for (size_t i = 0; i < res.constraint_count; ++i) {
        const auto& cons = model.get_constraint(static_cast<index_t>(i));
        real_t activity = 0.0;

        for (const auto& term : cons.terms) {
            if (term.first >= 0 && static_cast<size_t>(term.first) < res.variable_count) {
                activity += term.second * candidate_solution[term.first];
            }
        }

        real_t viol = 0.0;
        bool is_violated = false;

        if (cons.sense == ConstraintSense::LESS_EQUAL) {
            if (activity > cons.rhs + options.feasibility_tolerance) {
                viol = activity - cons.rhs;
                is_violated = true;
            }
        } else if (cons.sense == ConstraintSense::GREATER_EQUAL) {
            if (activity < cons.rhs - options.feasibility_tolerance) {
                viol = cons.rhs - activity;
                is_violated = true;
            }
        } else if (cons.sense == ConstraintSense::EQUAL) {
            real_t diff = std::abs(activity - cons.rhs);
            if (diff > options.feasibility_tolerance) {
                viol = diff;
                is_violated = true;
            }
        } else if (cons.sense == ConstraintSense::RANGED) {
            if (activity < cons.rhs - options.feasibility_tolerance) {
                viol = cons.rhs - activity;
                is_violated = true;
            } else if (activity > cons.range_upper + options.feasibility_tolerance) {
                viol = activity - cons.range_upper;
                is_violated = true;
            }
        }

        if (is_violated) {
            res.maximum_constraint_violation = std::max(res.maximum_constraint_violation, viol);
            res.violated_constraint_count++;
            res.verified = false;
            std::ostringstream oss;
            oss << "Constraint '" << cons.name << "' violated: activity=" << activity 
                << ", rhs=" << cons.rhs << ", violation=" << viol;
            res.diagnostic_messages.push_back(oss.str());
        }
    }

    // 4. Objective Recomputation
    real_t recomputed_obj = model.obj_offset();
    for (size_t j = 0; j < res.variable_count; ++j) {
        recomputed_obj += model.get_variable(static_cast<index_t>(j)).obj_coeff * candidate_solution[j];
    }
    res.objective_recomputed = recomputed_obj;

    if (res.verified) {
        res.status_message = "Candidate solution is mathematically valid and verified against original model.";
    } else {
        res.status_message = "Candidate solution failed independent mathematical verification.";
    }

    return res;
}

VerificationResult SolutionVerifier::verify(const LPModel& model,
                                            const std::vector<real_t>& candidate_solution,
                                            real_t reported_objective,
                                            const VerificationOptions& options) const {
    VerificationResult res = verify(model, candidate_solution, options);
    res.has_reported_objective = true;
    res.objective_reported = reported_objective;
    res.objective_difference = std::abs(res.objective_recomputed - reported_objective);

    if (options.check_objective && res.objective_difference > options.objective_tolerance) {
        res.verified = false;
        std::ostringstream oss;
        oss << "Reported objective (" << reported_objective << ") differs from recomputed objective (" 
            << res.objective_recomputed << ") by " << res.objective_difference 
            << " > tolerance (" << options.objective_tolerance << ").";
        res.diagnostic_messages.push_back(oss.str());
        res.status_message = "Candidate solution failed verification: objective mismatch.";
    }

    return res;
}

bool SolutionVerifier::verify_solution_feasibility(const LPModel& model,
                                                    const std::vector<real_t>& candidate_solution,
                                                    real_t tol) {
    SolutionVerifier verifier;
    VerificationOptions opts;
    opts.feasibility_tolerance = tol;
    opts.check_objective = false;
    VerificationResult res = verifier.verify(model, candidate_solution, opts);
    return res.verified;
}

} // namespace bharatopt
