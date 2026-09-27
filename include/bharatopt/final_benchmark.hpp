#ifndef BHARATOPT_FINAL_BENCHMARK_HPP
#define BHARATOPT_FINAL_BENCHMARK_HPP

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>
#include <bharatopt/solution_verifier.hpp>
#include <bharatopt/benchmark_framework.hpp>
#include <bharatopt/ablation_study.hpp>
#include <bharatopt/scalability_test.hpp>
#include <bharatopt/numerical_stress_test.hpp>

namespace bharatopt {

struct FinalBenchmarkRecord {
    std::string experiment_id;
    std::string category; // "SYNTHETIC_LP", "SYNTHETIC_MILP", "NETLIB", "MIPLIB", "SCALABILITY", "ABLATION", "NUMERICAL_STRESS"
    std::string instance_name;
    std::string problem_type{"LP"}; // "LP", "MILP"
    std::string source; // "SYNTHETIC", "NETLIB", "MIPLIB", "SCALABILITY", "ABLATION", "STRESS"

    size_t m{0};
    size_t n{0};
    size_t nnz{0};
    real_t density{0.0};

    std::string solver{"REVISED_SIMPLEX"};
    std::string routing_mode{"AUTO"};

    bool presolve_enabled{true};
    bool warm_start_enabled{false};

    std::string solver_status{"NOT_RUN"};
    std::string expected_status{"OPTIMAL"};
    bool status_match{false};

    real_t objective{0.0};
    real_t objective_difference{0.0};

    size_t iterations{0};
    size_t pivots{0};
    size_t nodes{0};
    size_t cuts{0};

    double solve_time_ms{0.0};
    double presolve_time_ms{0.0};
    double verification_time_ms{0.0};

    double peak_ram_mb{0.0};

    bool gpu_available{false};
    bool native_cuda{false};
    std::string gpu_name{"NOT_AVAILABLE"};
    double vram_used_mb{0.0};

    double h2d_ms{0.0};
    double kernel_ms{0.0};
    double d2h_ms{0.0};

    real_t max_constraint_violation{0.0};
    real_t max_bound_violation{0.0};
    real_t max_integrality_violation{0.0};

    std::string verification_status{"NOT_APPLICABLE"}; // "VERIFIED", "FAILED", "NOT_APPLICABLE"
    std::string comparison_outcome{"NOT_COMPARABLE"}; // "WIN", "LOSS", "TIE", "NOT_COMPARABLE", "NOT_AVAILABLE"
    std::string highs_status{"NOT_AVAILABLE"};

    std::string failure_reason;
    uint64_t seed{42};
};

class FinalBenchmarkSuite {
public:
    static FinalBenchmarkRecord ingest_synthetic_lp(uint64_t seed = 42);
    static FinalBenchmarkRecord ingest_synthetic_milp(uint64_t seed = 42);
    static FinalBenchmarkRecord ingest_netlib_sample(uint64_t seed = 42);
    static FinalBenchmarkRecord ingest_miplib_sample(uint64_t seed = 42);
    static FinalBenchmarkRecord ingest_scalability_record(const ScalabilityResultRecord& s_rec);
    static FinalBenchmarkRecord ingest_ablation_record(const AblationRecord& a_rec);
    static FinalBenchmarkRecord ingest_stress_record(const NumericalStressRecord& st_rec);

    static std::vector<FinalBenchmarkRecord> run_full_suite();
};

class FinalBenchmarkReporter {
public:
    static bool export_csv(const std::vector<FinalBenchmarkRecord>& records, const std::string& filepath);
    static bool export_json(const std::vector<FinalBenchmarkRecord>& records, const std::string& filepath);
    static void print_summary_table(const std::vector<FinalBenchmarkRecord>& records);
};

} // namespace bharatopt

#endif // BHARATOPT_FINAL_BENCHMARK_HPP
