#ifndef BHARATOPT_COST_ESTIMATOR_HPP
#define BHARATOPT_COST_ESTIMATOR_HPP

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>
#include <bharatopt/presolve.hpp>
#include <bharatopt/gpu_backend.hpp>
#include <bharatopt/benchmark_framework.hpp>

namespace bharatopt {

enum class ValidityStatus {
    VALID,
    UNKNOWN,
    INVALID
};

enum class ExtrapolationStatus {
    IN_DOMAIN,
    EXTRAPOLATION_WARNING
};

enum class ConfidenceLevel {
    HIGH_DATA_SUPPORT,
    LIMITED_DATA_SUPPORT,
    EXTRAPOLATION_WARNING
};

enum class GpuExecutionStatus {
    NATIVE_GPU,
    CPU_FALLBACK,
    GPU_UNAVAILABLE,
    INVALID
};

struct WorkloadFeatures {
    // Model dimensions
    size_t m{0};
    size_t n{0};
    size_t nnz{0};
    real_t density{0.0};
    real_t aspect_ratio{0.0}; // m / n
    real_t avg_nnz_per_row{0.0};
    real_t avg_nnz_per_col{0.0};

    // Numerical characteristics
    real_t coeff_min{0.0};
    real_t coeff_max{0.0};
    real_t dynamic_range{0.0};

    // Presolve characteristics
    size_t orig_m{0};
    size_t orig_n{0};
    size_t orig_nnz{0};
    size_t reduced_m{0};
    size_t reduced_n{0};
    size_t reduced_nnz{0};
    real_t var_reduction_ratio{0.0};
    real_t con_reduction_ratio{0.0};
    real_t nnz_reduction_ratio{0.0};
    double presolve_time_ms{0.0};

    // Hardware profile
    unsigned int cpu_core_count{1};
    unsigned int logical_processor_count{1};
    bool gpu_available{false};
    std::string gpu_name{"N/A"};
    size_t gpu_vram_mb{0};
    bool cuda_available{false};

    // Memory volume
    size_t estimated_matrix_memory_bytes{0};
    size_t estimated_vector_memory_bytes{0};
    size_t estimated_transfer_volume_bytes{0};

    // Metadata
    double extraction_time_ms{0.0};
    std::string feature_schema_version{"v1.0"};
    ValidityStatus validity{ValidityStatus::VALID};
};

struct CostEstimate {
    std::string solver_name;
    std::string solver_variant;
    
    double predicted_total_time_ms{0.0};
    double predicted_compute_time_ms{0.0};
    double predicted_transfer_time_ms{0.0};

    bool prediction_valid{true};
    ValidityStatus validity_status{ValidityStatus::VALID};
    ExtrapolationStatus extrapolation_status{ExtrapolationStatus::IN_DOMAIN};
    ConfidenceLevel confidence_status{ConfidenceLevel::HIGH_DATA_SUPPORT};
    GpuExecutionStatus gpu_execution_status{GpuExecutionStatus::NATIVE_GPU};

    std::string model_version{"v1.0-analytical"};
    std::string feature_version{"v1.0"};
    std::string hardware_profile{"Standard-CPU-GPU"};
    std::string extrapolation_message{"In calibration domain."};
};

/**
 * Problem Analyser: Extracts WorkloadFeatures before solving.
 */
class ProblemAnalyser {
public:
    static WorkloadFeatures extract_features(const LPModel& model, bool run_presolve = true);
};

/**
 * Extrapolation Detector: Verifies if WorkloadFeatures lie within calibration domain.
 */
class ExtrapolationDetector {
public:
    static ExtrapolationStatus check_domain(const WorkloadFeatures& features, std::string& warning_msg);
};

/**
 * Abstract Cost Estimator Interface.
 */
class ICostEstimator {
public:
    virtual ~ICostEstimator() = default;
    virtual CostEstimate estimate(const WorkloadFeatures& features, BenchmarkSolverType solver) const = 0;
    virtual std::vector<CostEstimate> estimate_all(const WorkloadFeatures& features) const = 0;
};

/**
 * Calibrated Analytical Cost Estimator.
 */
class AnalyticalCostEstimator : public ICostEstimator {
public:
    AnalyticalCostEstimator() = default;

    CostEstimate estimate(const WorkloadFeatures& features, BenchmarkSolverType solver) const override;
    std::vector<CostEstimate> estimate_all(const WorkloadFeatures& features) const override;
};

/**
 * Prediction Evaluator: Evaluates predictions against Phase 13 benchmark dataset.
 */
struct PredictionEvaluationRecord {
    std::string instance_id;
    std::string solver_name;
    double predicted_time_ms{0.0};
    double actual_time_ms{0.0};
    double absolute_error_ms{0.0};
    double relative_error{0.0};
    bool in_domain{true};
    std::string uncertainty_status;
    std::string model_version{"v1.0"};
    std::string dataset_version{"phase13_v1"};
};

struct CostEstimatorMetrics {
    double mae{0.0};                  // Mean Absolute Error (ms)
    double rmse{0.0};                 // Root Mean Square Error (ms)
    double median_absolute_error{0.0}; // Median Absolute Error (ms)
    double ranking_agreement_pct{0.0}; // % instances where predicted cost rank matches actual cost rank
    double avg_estimator_overhead_ms{0.0};
    size_t evaluated_instances{0};
};

class CostEstimatorEvaluator {
public:
    static CostEstimatorMetrics evaluate(
        const std::vector<BenchmarkResultRecord>& dataset,
        std::vector<PredictionEvaluationRecord>& out_records
    );

    static bool export_prediction_report_csv(
        const std::vector<PredictionEvaluationRecord>& records,
        const std::string& filepath
    );

    static bool export_prediction_report_json(
        const std::vector<PredictionEvaluationRecord>& records,
        const std::string& filepath
    );
};

} // namespace bharatopt

#endif // BHARATOPT_COST_ESTIMATOR_HPP
