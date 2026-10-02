#ifndef BHARATOPT_BENCHMARK_FRAMEWORK_HPP
#define BHARATOPT_BENCHMARK_FRAMEWORK_HPP

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <cstdint>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>
#include <bharatopt/presolve.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/dual_revised_simplex.hpp>
#include <bharatopt/first_order_solver.hpp>
#include <bharatopt/branch_and_bound.hpp>

namespace bharatopt {

enum class BenchmarkCategory {
    DIMENSION_SCALING,  // Category A
    SPARSITY_SCALING,   // Category B
    NNZ_SCALING,        // Category C
    MATRIX_SHAPE,       // Category D
    COEFFICIENT_SCALE,  // Category E
    PRESOLVE_IMPACT,    // Category F
    STRUCTURED_LP       // Category G
};

enum class BenchmarkSolverType {
    REVISED_SIMPLEX,
    DUAL_REVISED_SIMPLEX,
    CPU_FIRST_ORDER,
    GPU_FIRST_ORDER,
    EDUCATIONAL_SIMPLEX,
    BRANCH_AND_BOUND
};

struct BenchmarkInstanceConfig {
    std::string instance_id;
    BenchmarkCategory category{BenchmarkCategory::DIMENSION_SCALING};
    size_t num_variables{50};
    size_t num_constraints{25};
    real_t target_density{0.05}; // NNZ / (m * n)
    uint64_t seed{42};
    real_t coeff_min{0.1};
    real_t coeff_max{10.0};
    std::string shape_type{"square"}; // "square", "tall", "wide", "banded", "diagonal", "block_sparse"
    bool presolve_enabled{true};
    ObjectiveSense sense{ObjectiveSense::MAXIMIZE};
};

struct BenchmarkResultRecord {
    std::string benchmark_id;
    std::string instance_id;
    std::string solver_name;
    std::string solver_variant;
    
    // Model characteristics
    size_t rows{0};
    size_t cols{0};
    size_t nnz{0};
    real_t density{0.0};
    std::string shape;
    real_t coeff_min{0.0};
    real_t coeff_max{0.0};
    
    // Presolve metrics
    bool presolve_enabled{false};
    double presolve_time_ms{0.0};
    size_t vars_removed{0};
    size_t cons_removed{0};
    size_t reduced_rows{0};
    size_t reduced_cols{0};
    size_t reduced_nnz{0};
    
    // Solver execution metrics
    std::string status;
    double solve_time_ms{0.0};
    size_t iterations{0};
    size_t refactorisations{0};
    size_t eta_updates{0};
    
    // GPU specific metrics
    double h2d_time_ms{0.0};
    double kernel_time_ms{0.0};
    double d2h_time_ms{0.0};
    double sync_time_ms{0.0};
    double total_gpu_time_ms{0.0};
    
    // Numerical & Verification metrics
    real_t objective_value{0.0};
    real_t max_residual{0.0};
    real_t constraint_violation{0.0};
    real_t bound_violation{0.0};
    bool verification_passed{false};
    
    // Execution metadata
    uint64_t seed{0};
    size_t repetitions{1};
    std::string timestamp;
    double total_time_ms{0.0};
};

// ============================================================
// PHASE 18: MILP BENCHMARK MATRIX INFRASTRUCTURE
// ============================================================

enum class MilpBenchmarkCategory {
    M1_DIMENSION_SCALING,
    M2_INTEGER_DENSITY,
    M3_BINARY_VS_INTEGER,
    M4_SPARSITY,
    M5_INTEGER_FRACTIONALITY,
    M6_TREE_GROWTH,
    M7_OBJECTIVE_SENSE,
    M8_NUMERICAL_SCALE,
    M9_HAND_DERIVED,
    M10_BRUTE_FORCE_CROSS_CHECK
};

struct MilpBenchmarkInstanceConfig {
    std::string instance_id;
    MilpBenchmarkCategory category{MilpBenchmarkCategory::M1_DIMENSION_SCALING};
    size_t num_variables{20};
    size_t num_constraints{10};
    real_t target_density{0.10};
    uint64_t seed{42};
    real_t coeff_min{0.1};
    real_t coeff_max{10.0};
    real_t integer_fraction{0.50}; // Fraction of variables that are non-continuous
    real_t binary_ratio{0.50};     // Ratio of integer variables that are binary
    ObjectiveSense sense{ObjectiveSense::MAXIMIZE};
    size_t max_nodes{1000};
    std::string fractionality_type{"mildly_fractional"}; // "integer_root", "mildly_fractional", "substantially_fractional"
};

struct MilpBenchmarkResultRecord {
    std::string benchmark_id;
    std::string instance_id;
    std::string category_name;
    
    // Model characteristics
    size_t rows{0};
    size_t cols{0};
    size_t nnz{0};
    real_t density{0.0};
    size_t continuous_count{0};
    size_t integer_count{0};
    size_t binary_count{0};
    std::string sense;
    real_t coeff_min{0.0};
    real_t coeff_max{0.0};
    real_t dynamic_range{1.0};
    
    // B&B telemetry
    std::string status;
    double total_solve_time_ms{0.0};
    double root_lp_time_ms{0.0};
    size_t nodes_created{0};
    size_t nodes_processed{0};
    size_t max_depth{0};
    size_t nodes_pruned_infeasibility{0};
    size_t nodes_pruned_bound{0};
    size_t integer_feasible_nodes{0};
    size_t incumbent_updates{0};
    real_t final_incumbent_objective{0.0};
    real_t best_remaining_bound{0.0};
    bool node_limit_reached{false};
    bool numerical_failures{false};
    
    // LP relaxation breakdown
    size_t lp_solve_count{0};
    double cumulative_lp_time_ms{0.0};
    double avg_lp_time_ms{0.0};
    double min_lp_time_ms{0.0};
    double max_lp_time_ms{0.0};
    
    // Independent verification
    bool verification_passed{false};
    real_t max_residual{0.0};
    real_t constraint_violation{0.0};
    real_t bound_violation{0.0};
    real_t integrality_violation{0.0};
    real_t recomputed_objective{0.0};
    
    // Brute-force cross-check (small models)
    bool brute_force_evaluated{false};
    real_t brute_force_optimum{0.0};
    bool brute_force_matched{false};
    
    uint64_t seed{0};
};

/**
 * Deterministic Seed-Based Benchmark LP Instance Generator.
 */
class BenchmarkGenerator {
public:
    static LPModel generate_instance(const BenchmarkInstanceConfig& config);
};

/**
 * Single Instance Benchmark Execution Runner.
 */
class BenchmarkRunner {
public:
    static BenchmarkResultRecord run_benchmark(
        const BenchmarkInstanceConfig& config,
        BenchmarkSolverType solver_type,
        size_t repetitions = 1
    );
};

/**
 * Controlled LP Workload Benchmark Suite.
 */
class BenchmarkSuite {
public:
    BenchmarkSuite() = default;
    
    void add_config(const BenchmarkInstanceConfig& config);
    void add_default_workload_matrix();
    
    std::vector<BenchmarkResultRecord> run_suite(
        const std::vector<BenchmarkSolverType>& solvers = {
            BenchmarkSolverType::REVISED_SIMPLEX,
            BenchmarkSolverType::DUAL_REVISED_SIMPLEX,
            BenchmarkSolverType::CPU_FIRST_ORDER,
            BenchmarkSolverType::GPU_FIRST_ORDER
        },
        size_t repetitions = 1
    );
    
    const std::vector<BenchmarkInstanceConfig>& configs() const { return configs_; }

private:
    std::vector<BenchmarkInstanceConfig> configs_;
};

// ============================================================
// PHASE 18: MILP BENCHMARK GENERATOR, RUNNER & BRUTE FORCE
// ============================================================

class MilpBenchmarkGenerator {
public:
    static LPModel generate_instance(const MilpBenchmarkInstanceConfig& config);
};

struct BruteForceMilpResult {
    bool found_solution{false};
    real_t optimum_objective{0.0};
    std::vector<real_t> solution;
    uint64_t evaluated_count{0};
};

class BruteForceMilpSolver {
public:
    static BruteForceMilpResult solve(const LPModel& model);
};

class MilpBenchmarkRunner {
public:
    static MilpBenchmarkResultRecord run_milp_benchmark(const MilpBenchmarkInstanceConfig& config);
    static MilpBenchmarkResultRecord run_milp_benchmark_on_model(
        const std::string& instance_id,
        MilpBenchmarkCategory category,
        const LPModel& model,
        uint64_t seed = 42,
        size_t max_nodes = 1000
    );
};

struct MilpWarmStartResultRecord {
    std::string benchmark_id;
    std::string instance_id;
    std::string category_name;

    size_t rows{0};
    size_t cols{0};
    size_t nnz{0};
    size_t integer_count{0};
    size_t binary_count{0};

    // Cold Start Metrics
    double cold_time_ms{0.0};
    size_t cold_nodes_processed{0};
    size_t cold_nodes_created{0};
    real_t cold_objective{0.0};
    std::string cold_status;

    // Warm Start Metrics
    double warm_time_ms{0.0};
    size_t warm_nodes_processed{0};
    size_t warm_nodes_created{0};
    real_t warm_objective{0.0};
    std::string warm_status;

    // Warm Start Telemetry
    size_t warm_starts_attempted{0};
    size_t warm_starts_accepted{0};
    size_t warm_starts_rejected{0};
    size_t warm_starts_failed{0};
    size_t cold_fallbacks{0};

    // Evaluation
    double speedup{1.0};
    double time_diff_ms{0.0};
    bool correctness_matched{false};
    bool verification_passed{false};
};

class MilpBenchmarkSuite {
public:
    MilpBenchmarkSuite() = default;
    
    void add_config(const MilpBenchmarkInstanceConfig& config);
    void add_default_milp_matrix();
    
    std::vector<MilpBenchmarkResultRecord> run_suite();
    std::vector<MilpWarmStartResultRecord> run_warm_start_suite();
    
    const std::vector<MilpBenchmarkInstanceConfig>& configs() const { return configs_; }

private:
    std::vector<MilpBenchmarkInstanceConfig> configs_;
};

/**
 * Machine-Readable Result Exporter and Reporter (CSV / JSON).
 */
class BenchmarkReporter {
public:
    static bool export_csv(const std::vector<BenchmarkResultRecord>& results, const std::string& filepath);
    static bool export_json(const std::vector<BenchmarkResultRecord>& results, const std::string& filepath);
    static std::vector<BenchmarkResultRecord> import_csv(const std::string& filepath);
    static void print_summary_table(const std::vector<BenchmarkResultRecord>& results);
    
    // Phase 18 MILP Exporters
    static bool export_milp_csv(const std::vector<MilpBenchmarkResultRecord>& results, const std::string& filepath);
    static bool export_milp_json(const std::vector<MilpBenchmarkResultRecord>& results, const std::string& filepath);
    static void print_milp_summary_table(const std::vector<MilpBenchmarkResultRecord>& results);

    // Phase 19 Warm Start Exporters
    static bool export_warm_start_csv(const std::vector<MilpWarmStartResultRecord>& results, const std::string& filepath);
    static bool export_warm_start_json(const std::vector<MilpWarmStartResultRecord>& results, const std::string& filepath);
    static void print_warm_start_summary_table(const std::vector<MilpWarmStartResultRecord>& results);
};

} // namespace bharatopt

#endif // BHARATOPT_BENCHMARK_FRAMEWORK_HPP
