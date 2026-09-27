#ifndef BHARATOPT_FIRST_ORDER_SOLVER_HPP
#define BHARATOPT_FIRST_ORDER_SOLVER_HPP

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>
#include <bharatopt/sparse_matrix.hpp>
#include <bharatopt/gpu_backend.hpp>

namespace bharatopt {

enum class FirstOrderSolverStatus {
    OPTIMAL,
    MAX_ITERATIONS,
    NUMERICAL_FAILURE,
    INFEASIBLE_DETECTED,
    UNBOUNDED_DETECTED,
    INTERRUPTED
};

enum class FirstOrderBackendType {
    CPU_FIRST_ORDER,
    GPU_FIRST_ORDER
};

struct FirstOrderSolverOptions {
    size_t max_iterations{50000};
    real_t feasibility_tolerance{DEFAULT_FEASIBILITY_TOLERANCE};
    real_t optimality_tolerance{DEFAULT_OPTIMALITY_TOLERANCE};
    real_t primal_step_size{0.01};
    real_t dual_step_size{0.01};
    bool adaptive_step_size{true};
    FirstOrderBackendType backend_type{FirstOrderBackendType::CPU_FIRST_ORDER};
    size_t check_frequency{100};
};

struct FirstOrderSolverStats {
    size_t iterations{0};
    real_t primal_objective{0.0};
    real_t dual_objective{0.0};
    real_t primal_residual{0.0};
    real_t dual_residual{0.0};
    real_t constraint_violation{0.0};
    real_t bound_violation{0.0};
    double solve_time_ms{0.0};
    double transfer_time_ms{0.0};
    double kernel_time_ms{0.0};
};

struct FirstOrderSolverResult {
    FirstOrderSolverStatus status{FirstOrderSolverStatus::NUMERICAL_FAILURE};
    std::vector<real_t> primal_solution; // Original space
    std::vector<real_t> dual_solution;   // Constraint duals
    real_t objective_value{0.0};
    FirstOrderSolverStats stats;
    std::string message;
};

/**
 * Base Abstract Class for PDHG / PDLP-Style First-Order LP Solvers.
 */
class IFirstOrderLPSolver {
public:
    virtual ~IFirstOrderLPSolver() = default;
    virtual FirstOrderSolverResult solve(const LPModel& model) = 0;
};

/**
 * CPU Implementation of PDHG / PDLP-Style Primal-Dual First-Order Solver.
 */
class CPUFirstOrderSolver : public IFirstOrderLPSolver {
public:
    explicit CPUFirstOrderSolver(FirstOrderSolverOptions options = {});
    FirstOrderSolverResult solve(const LPModel& model) override;

    const FirstOrderSolverOptions& options() const { return options_; }

private:
    FirstOrderSolverOptions options_;
};

/**
 * GPU Implementation of PDHG / PDLP-Style Primal-Dual First-Order Solver.
 * Keeps vectors on device across iterations to eliminate per-iteration transfer overhead.
 */
class GPUFirstOrderSolver : public IFirstOrderLPSolver {
public:
    explicit GPUFirstOrderSolver(FirstOrderSolverOptions options = {});
    FirstOrderSolverResult solve(const LPModel& model) override;

    const FirstOrderSolverOptions& options() const { return options_; }

private:
    FirstOrderSolverOptions options_;
};

/**
 * High-Level First-Order LP Solver Dispatcher.
 */
class FirstOrderLPSolver {
public:
    explicit FirstOrderLPSolver(FirstOrderSolverOptions options = {});
    FirstOrderSolverResult solve(const LPModel& model);

    const FirstOrderSolverOptions& options() const { return options_; }
    void set_options(const FirstOrderSolverOptions& options) { options_ = options; }

private:
    FirstOrderSolverOptions options_;
};

} // namespace bharatopt

#endif // BHARATOPT_FIRST_ORDER_SOLVER_HPP
