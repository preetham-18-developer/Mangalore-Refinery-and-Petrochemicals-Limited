#ifndef BHARATOPT_SCALABILITY_TEST_HPP
#define BHARATOPT_SCALABILITY_TEST_HPP

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>
#include <bharatopt/sparse_matrix.hpp>

namespace bharatopt {

struct ScalabilityResultRecord {
    std::string experiment_id;
    std::string category{"SPARSE"};
    size_t m{0};
    size_t n{0};
    size_t nnz{0};
    real_t density{0.0};
    std::string representation{"CSC"};

    double generation_time_ms{0.0};
    double construction_time_ms{0.0};

    double baseline_ram_mb{0.0};
    double peak_ram_mb{0.0};
    double matrix_memory_mb{0.0};
    double vector_memory_mb{0.0};
    double total_memory_mb{0.0};

    bool gpu_available{false};
    bool native_cuda{false};
    std::string gpu_name{"NOT_AVAILABLE"};
    double vram_total_mb{0.0};
    double vram_used_mb{0.0};

    double h2d_ms{0.0};
    double kernel_ms{0.0};
    double d2h_ms{0.0};
    double gpu_total_ms{0.0};

    double cpu_execution_ms{0.0};
    std::string solver_status{"NOT_RUN"};
    std::string execution_status{"COMPLETED"}; // "COMPLETED", "SKIPPED_RESOURCE_LIMIT", "CUDA_UNAVAILABLE", "CPU_FALLBACK"
    std::string failure_reason;

    uint64_t seed{42};
    std::string timestamp;
};

class ProcessMemoryProfiler {
public:
    static double get_current_process_ram_mb();
    static double get_peak_process_ram_mb();
};

class ScalabilityHarness {
public:
    static ScalabilityResultRecord run_scalability_experiment(
        size_t n,
        size_t m,
        size_t nnz_per_col = 5,
        uint64_t seed = 42,
        double max_ram_mb = 8192.0
    );

    static std::vector<ScalabilityResultRecord> run_full_suite(
        double max_ram_mb = 8192.0
    );
};

class ScalabilityReporter {
public:
    static bool export_csv(const std::vector<ScalabilityResultRecord>& records, const std::string& filepath);
    static bool export_json(const std::vector<ScalabilityResultRecord>& records, const std::string& filepath);
    static void print_summary_table(const std::vector<ScalabilityResultRecord>& records);
};

} // namespace bharatopt

#endif // BHARATOPT_SCALABILITY_TEST_HPP
