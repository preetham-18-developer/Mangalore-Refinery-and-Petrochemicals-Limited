#ifndef BHARATOPT_EXECUTION_ROUTER_HPP
#define BHARATOPT_EXECUTION_ROUTER_HPP

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>
#include <bharatopt/presolve.hpp>
#include <bharatopt/gpu_backend.hpp>
#include <bharatopt/cost_estimator.hpp>
#include <bharatopt/benchmark_framework.hpp>

namespace bharatopt {

enum class RoutingMode {
    ADAPTIVE,
    FORCE_CPU_REVISED,
    FORCE_CPU_DUAL,
    FORCE_CPU_FIRST_ORDER,
    FORCE_GPU_FIRST_ORDER
};

struct RoutingConfiguration {
    RoutingMode routing_mode{RoutingMode::ADAPTIVE};
    bool gpu_allowed{true};
    bool fallback_enabled{true};
    bool presolve_enabled{true};
    double min_gpu_speedup_margin_ms{1.0}; // Required predicted savings to switch to GPU
};

struct RoutingDecision {
    BenchmarkSolverType selected_solver{BenchmarkSolverType::DUAL_REVISED_SIMPLEX};
    std::string solver_name{"DualRevisedSimplex"};
    std::string solver_variant{"CPU_Dual_Simplex"};
    std::string execution_device{"CPU"};

    double predicted_cpu_cost_ms{0.0};
    double predicted_gpu_cost_ms{0.0};
    double selected_predicted_cost_ms{0.0};

    std::string routing_reason;
    ConfidenceLevel confidence_status{ConfidenceLevel::HIGH_DATA_SUPPORT};
    ExtrapolationStatus extrapolation_status{ExtrapolationStatus::IN_DOMAIN};
    
    bool gpu_available{false};
    bool cuda_available{false};
    bool fallback_allowed{true};
    
    std::string estimator_version{"v1.0-analytical"};
    std::string feature_version{"v1.0"};
    double decision_time_ms{0.0};
};

struct RoutedSolveResult {
    RoutingDecision decision;
    std::string status{"NUMERICAL_FAILURE"};
    
    real_t objective_value{0.0};
    std::vector<real_t> primal_solution;
    std::vector<real_t> dual_solution;

    double total_time_ms{0.0};
    double presolve_time_ms{0.0};
    double routing_overhead_ms{0.0};
    double actual_solve_time_ms{0.0};
    size_t iterations{0};

    bool verification_passed{false};
    real_t max_residual{0.0};

    bool fallback_occurred{false};
    std::string fallback_reason;
    std::string fallback_solver_name;
    double fallback_solve_time_ms{0.0};

    BnBTelemetry bnb_telemetry;
};

/**
 * Abstract Execution Router Interface.
 */
class IExecutionRouter {
public:
    virtual ~IExecutionRouter() = default;
    
    virtual WorkloadFeatures analyse(const LPModel& model) = 0;
    virtual std::vector<CostEstimate> estimate(const WorkloadFeatures& features) = 0;
    virtual RoutingDecision decide(const LPModel& model, const WorkloadFeatures& features, const std::vector<CostEstimate>& estimates) = 0;
    virtual RoutedSolveResult solve(const LPModel& model) = 0;
};

/**
 * Adaptive CPU/GPU Execution Router.
 */
class ExecutionRouter : public IExecutionRouter {
public:
    explicit ExecutionRouter(
        RoutingConfiguration config = {},
        std::shared_ptr<ICostEstimator> estimator = nullptr
    );

    WorkloadFeatures analyse(const LPModel& model) override;
    std::vector<CostEstimate> estimate(const WorkloadFeatures& features) override;
    RoutingDecision decide(const LPModel& model, const WorkloadFeatures& features, const std::vector<CostEstimate>& estimates) override;
    RoutedSolveResult solve(const LPModel& model) override;

    const RoutingConfiguration& config() const { return config_; }
    void set_config(const RoutingConfiguration& config) { config_ = config; }

private:
    RoutingConfiguration config_;
    std::shared_ptr<ICostEstimator> estimator_;
};

/**
 * Router Evaluation Telemetry Record.
 */
struct RoutingEvaluationRecord {
    std::string instance_id;
    std::string selected_solver;
    std::string selected_device;
    double predicted_cpu_cost_ms{0.0};
    double predicted_gpu_cost_ms{0.0};
    double actual_solve_time_ms{0.0};
    double actual_best_eligible_time_ms{0.0};
    double regret_ms{0.0};
    bool optimal_path_selected{false};
    std::string routing_reason;
    bool fallback_occurred{false};
    bool verification_passed{false};
};

struct RouterEvaluationMetrics {
    size_t total_evaluated_instances{0};
    double selection_agreement_pct{0.0}; // % instances where adaptive router selected fastest eligible path
    double mean_regret_ms{0.0};          // Average (selected_time - min_eligible_time)
    double max_regret_ms{0.0};
    double avg_routing_overhead_ms{0.0};
    size_t native_gpu_selections{0};
    size_t cpu_selections{0};
    size_t fallback_events{0};
};

class RouterEvaluator {
public:
    static RouterEvaluationMetrics evaluate(
        const std::vector<BenchmarkResultRecord>& dataset,
        std::vector<RoutingEvaluationRecord>& out_records
    );

    static bool export_routing_report_csv(
        const std::vector<RoutingEvaluationRecord>& records,
        const std::string& filepath
    );

    static bool export_routing_report_json(
        const std::vector<RoutingEvaluationRecord>& records,
        const std::string& filepath
    );
};

} // namespace bharatopt

#endif // BHARATOPT_EXECUTION_ROUTER_HPP
