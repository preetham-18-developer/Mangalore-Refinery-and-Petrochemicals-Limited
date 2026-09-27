#ifndef BHARATOPT_NUMERICAL_STRESS_TEST_HPP
#define BHARATOPT_NUMERICAL_STRESS_TEST_HPP

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/dual_revised_simplex.hpp>
#include <bharatopt/solution_verifier.hpp>

namespace bharatopt {

struct NumericalStressRecord {
    std::string experiment_id;
    std::string category; // "ILL_CONDITIONED", "DEGENERATE", "UNBOUNDED", "INFEASIBLE", "EXTREME_SCALE"
    uint64_t seed{42};

    size_t m{0};
    size_t n{0};
    size_t nnz{0};

    real_t coefficient_min{0.0};
    real_t coefficient_max{0.0};
    real_t coefficient_dynamic_range{1.0};

    real_t condition_metric{1.0};
    bool presolve_enabled{false};
    size_t presolve_reductions{0};

    real_t pivot_tolerance{DEFAULT_PIVOT_TOLERANCE};
    real_t feasibility_tolerance{DEFAULT_FEASIBILITY_TOLERANCE};
    real_t optimality_tolerance{DEFAULT_OPTIMALITY_TOLERANCE};

    std::string solver{"REVISED_SIMPLEX"};
    std::string solver_status{"NOT_RUN"};
    std::string expected_status{"OPTIMAL"};
    bool status_match{false};

    size_t iterations{0};
    size_t pivot_count{0};

    real_t objective{0.0};
    real_t max_constraint_violation{0.0};
    real_t max_bound_violation{0.0};
    real_t max_integrality_violation{0.0};

    std::string verification_status{"NOT_APPLICABLE"}; // "VERIFIED", "FAILED", "NOT_APPLICABLE"

    double solve_time_ms{0.0};
    double presolve_time_ms{0.0};

    bool numerical_failure{false};
    std::string failure_reason;

    bool native_cuda{false};
    std::string gpu_status{"CPU_FALLBACK"};
};

class NumericalStressGenerator {
public:
    // Category A: Ill-conditioned LP (Hilbert / Vandermonde matrix structure)
    static LPModel generate_ill_conditioned_lp(size_t dim = 5, uint64_t seed = 42);

    // Category B: Degenerate LP (multiple zero basic variables / zero reduced costs)
    static LPModel generate_degenerate_lp(size_t dim = 6, uint64_t seed = 42);

    // Category C: Unbounded LP (mathematically unbounded ray verified prior to solver)
    static LPModel generate_unbounded_lp(size_t vars = 4, uint64_t seed = 42);

    // Category D: Infeasible LP (contradictory constraint set verified prior to solver)
    static LPModel generate_infeasible_lp(size_t vars = 4, uint64_t seed = 42);

    // Category E: Extreme Coefficient Scale LP (range 10^9 down to 10^-12)
    static LPModel generate_extreme_scale_lp(uint64_t seed = 42);
};

class NumericalStressHarness {
public:
    static NumericalStressRecord evaluate_stress_case(
        const std::string& exp_id,
        const std::string& category,
        const LPModel& model,
        const std::string& expected_status,
        bool enable_presolve = false,
        real_t pivot_tol = DEFAULT_PIVOT_TOLERANCE,
        uint64_t seed = 42
    );

    static std::vector<NumericalStressRecord> run_full_suite();
};

class NumericalStressReporter {
public:
    static bool export_csv(const std::vector<NumericalStressRecord>& records, const std::string& filepath);
    static bool export_json(const std::vector<NumericalStressRecord>& records, const std::string& filepath);
    static void print_summary_table(const std::vector<NumericalStressRecord>& records);
};

} // namespace bharatopt

#endif // BHARATOPT_NUMERICAL_STRESS_TEST_HPP
