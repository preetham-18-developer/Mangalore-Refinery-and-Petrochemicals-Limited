#ifndef BHARATOPT_SOLUTION_VERIFIER_HPP
#define BHARATOPT_SOLUTION_VERIFIER_HPP

#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>

namespace bharatopt {

struct VerificationOptions {
    real_t feasibility_tolerance{DEFAULT_FEASIBILITY_TOLERANCE};
    real_t integrality_tolerance{DEFAULT_INTEGRALITY_TOLERANCE};
    real_t objective_tolerance{1e-4};
    bool check_objective{true};
};

struct VerificationResult {
    bool verified{false};
    std::string status_message;

    bool has_reported_objective{false};
    real_t objective_reported{0.0};
    real_t objective_recomputed{0.0};
    real_t objective_difference{0.0};

    real_t maximum_constraint_violation{0.0};
    real_t maximum_lower_bound_violation{0.0};
    real_t maximum_upper_bound_violation{0.0};
    real_t maximum_integrality_violation{0.0};

    size_t violated_constraint_count{0};
    size_t violated_bound_count{0};
    size_t violated_integrality_count{0};

    size_t variable_count{0};
    size_t constraint_count{0};

    std::vector<std::string> diagnostic_messages;

    std::string to_string() const;
};

class SolutionVerifier {
public:
    explicit SolutionVerifier(VerificationOptions options = VerificationOptions());

    VerificationResult verify(const LPModel& model,
                              const std::vector<real_t>& candidate_solution,
                              const VerificationOptions& options = VerificationOptions()) const;

    VerificationResult verify(const LPModel& model,
                              const std::vector<real_t>& candidate_solution,
                              real_t reported_objective,
                              const VerificationOptions& options = VerificationOptions()) const;

    static bool verify_solution_feasibility(const LPModel& model,
                                            const std::vector<real_t>& candidate_solution,
                                            real_t tol = DEFAULT_FEASIBILITY_TOLERANCE);

private:
    VerificationOptions default_options_;
};

} // namespace bharatopt

#endif // BHARATOPT_SOLUTION_VERIFIER_HPP
