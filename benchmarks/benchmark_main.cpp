#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <bharatopt/version.hpp>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>
#include <bharatopt/model_validator.hpp>
#include <bharatopt/sparse_matrix.hpp>
#include <bharatopt/presolve.hpp>
#include <bharatopt/educational_simplex.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/dual_revised_simplex.hpp>
#include <bharatopt/basis_update.hpp>
#include <bharatopt/gpu_backend.hpp>
#include <bharatopt/first_order_solver.hpp>
#include <bharatopt/benchmark_framework.hpp>
#include <bharatopt/cost_estimator.hpp>
#include <bharatopt/execution_router.hpp>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    try {
        std::cout << "========================================================\n";
        std::cout << " BharatOpt Performance & Micro-Benchmark Suite\n";
        std::cout << " " << bharatopt::get_version_info() << "\n";
        std::cout << "========================================================\n\n" << std::flush;

    // 1. Baseline Vector Addition Benchmark
    constexpr std::size_t N = 1000000;
    std::vector<bharatopt::real_t> vec_a(N, 1.000001);
    std::vector<bharatopt::real_t> vec_b(N, 2.000002);
    std::vector<bharatopt::real_t> vec_c(N, 0.0);

    std::cout << "[BENCHMARK 1] Baseline CPU Vector Addition (N = " << N << ")...\n";
    auto start1 = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < N; ++i) {
        vec_c[i] = vec_a[i] + vec_b[i];
    }

    auto end1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration1 = end1 - start1;
    std::cout << "[RESULT] Vector Addition Time: " << duration1.count() << " ms\n\n";

    // 2. ModelValidator Performance Benchmark
    std::cout << "[BENCHMARK 2] ModelValidator on Synthetic Model (Vars = 10,000, Cons = 5,000, NNZ = 50,000)...\n";
    bharatopt::LPModel synth_model("synthetic_benchmark_model");

    std::vector<bharatopt::index_t> var_indices;
    var_indices.reserve(10000);
    for (int i = 0; i < 10000; ++i) {
        var_indices.push_back(synth_model.add_variable("x_" + std::to_string(i), 0.0, 100.0, 1.5));
    }

    for (int c = 0; c < 5000; ++c) {
        std::vector<std::pair<bharatopt::index_t, bharatopt::real_t>> terms;
        for (int t = 0; t < 10; ++t) {
            bharatopt::index_t var_idx = var_indices[(c * 10 + t) % 10000];
            terms.push_back({var_idx, static_cast<bharatopt::real_t>(t + 1)});
        }
        synth_model.add_constraint("c_" + std::to_string(c), terms, bharatopt::ConstraintSense::LESS_EQUAL, 50.0);
    }

    bharatopt::ModelValidator validator;
    auto start2 = std::chrono::high_resolution_clock::now();
    bharatopt::ValidationResult val_res = validator.validate(synth_model);
    auto end2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration2 = end2 - start2;
    std::cout << "[RESULT] Model Validation Time: " << duration2.count() << " ms\n\n";

    // 3. Sparse Matrix SpMV & Conversion Benchmarks
    std::cout << "[BENCHMARK 3] Sparse Matrix Engine (Rows = 10,000, Cols = 10,000, NNZ = 100,000)...\n";
    bharatopt::COOMatrix coo(10000, 10000);
    for (int i = 0; i < 10000; ++i) {
        bharatopt::index_t r = static_cast<bharatopt::index_t>(i);
        for (int k = 0; k < 10; ++k) {
            bharatopt::index_t c = static_cast<bharatopt::index_t>((i * 7 + k * 997) % 10000);
            coo.add_entry(r, c, static_cast<bharatopt::real_t>(k + 1));
        }
    }

    auto t_conv_csr_start = std::chrono::high_resolution_clock::now();
    bharatopt::CSRMatrix csr = bharatopt::CSRMatrix::from_coo(coo);
    auto t_conv_csr_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_conv_csr = t_conv_csr_end - t_conv_csr_start;
    std::cout << "[RESULT] COO -> CSR Conversion Time: " << duration_conv_csr.count() << " ms\n";

    std::vector<bharatopt::real_t> x(10000, 1.0);
    std::vector<bharatopt::real_t> y_csr(10000, 0.0);
    constexpr int ITERS = 100;
    csr.multiply(x.data(), y_csr.data()); // Warm up

    auto t_csr_spmv_start = std::chrono::high_resolution_clock::now();
    for (int iter = 0; iter < ITERS; ++iter) {
        csr.multiply(x.data(), y_csr.data());
    }
    auto t_csr_spmv_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_csr_spmv = (t_csr_spmv_end - t_csr_spmv_start) / ITERS;
    std::cout << "[RESULT] CSR SpMV Average Execution Time: " << duration_csr_spmv.count() << " ms / iter\n\n";

    // 4. PresolveEngine Performance Benchmark
    std::cout << "[BENCHMARK 4] PresolveEngine on Medium Reducible Model (Vars = 5,000, Cons = 3,000)...\n";
    bharatopt::LPModel presolve_model("presolve_benchmark_model");

    // Add 5,000 vars (1,000 fixed, 500 empty)
    for (int i = 0; i < 1000; ++i) {
        presolve_model.add_variable("fixed_" + std::to_string(i), 5.0, 5.0, 2.0); // Fixed
    }
    for (int i = 0; i < 500; ++i) {
        presolve_model.add_variable("empty_" + std::to_string(i), 0.0, 10.0, 0.0); // Empty col
    }
    std::vector<bharatopt::index_t> active_v;
    for (int i = 0; i < 3500; ++i) {
        active_v.push_back(presolve_model.add_variable("act_" + std::to_string(i), 0.0, 100.0, 1.0));
    }

    for (int c = 0; c < 3000; ++c) {
        std::vector<std::pair<bharatopt::index_t, bharatopt::real_t>> terms;
        for (int t = 0; t < 5; ++t) {
            terms.push_back({active_v[(c * 3 + t) % 3500], static_cast<bharatopt::real_t>(t + 1)});
        }
        presolve_model.add_constraint("c_" + std::to_string(c), terms, bharatopt::ConstraintSense::LESS_EQUAL, 100.0);
    }

    bharatopt::PresolveEngine presolve_engine;
    auto t_presolve_start = std::chrono::high_resolution_clock::now();
    bharatopt::PresolveResult presolve_res = presolve_engine.presolve(presolve_model);
    auto t_presolve_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_presolve = t_presolve_end - t_presolve_start;

    std::cout << "[RESULT] Presolve Execution Time: " << duration_presolve.count() << " ms\n";
    std::cout << "[RESULT] " << presolve_res.stats.vars_removed << " Vars Removed, "
              << presolve_res.stats.cons_removed << " Cons Removed\n\n";

    // 5. EducationalSimplex Micro-Benchmark
    std::cout << "[BENCHMARK 5] EducationalSimplex Reference Solver (Vars = 100, Cons = 50)...\n";
    bharatopt::LPModel simplex_bench_model("simplex_bench");
    simplex_bench_model.set_sense(bharatopt::ObjectiveSense::MAXIMIZE);
    std::vector<bharatopt::index_t> s_vars;
    for (int j = 0; j < 100; ++j) {
        s_vars.push_back(simplex_bench_model.add_variable("x_" + std::to_string(j), 0.0, 100.0, static_cast<bharatopt::real_t>((j % 7) + 1)));
    }
    for (int i = 0; i < 50; ++i) {
        std::vector<std::pair<bharatopt::index_t, bharatopt::real_t>> terms;
        for (int k = 0; k < 5; ++k) {
            terms.push_back({s_vars[(i * 3 + k) % 100], static_cast<bharatopt::real_t>(k + 1)});
        }
        simplex_bench_model.add_constraint("c_" + std::to_string(i), terms, bharatopt::ConstraintSense::LESS_EQUAL, 50.0 + i);
    }

    bharatopt::EducationalSimplex simplex_solver;
    auto t_simplex_start = std::chrono::high_resolution_clock::now();
    bharatopt::SimplexResult simplex_res = simplex_solver.solve(simplex_bench_model);
    auto t_simplex_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_simplex = t_simplex_end - t_simplex_start;

    std::cout << "[RESULT] EducationalSimplex Execution Time: " << duration_simplex.count() << " ms\n";
    std::cout << "[RESULT] Solved in " << simplex_res.iterations << " Pivots with Status: "
              << (simplex_res.status == bharatopt::SimplexStatus::OPTIMAL ? "OPTIMAL" : "OTHER") << "\n\n";

    // 6. RevisedSimplex Micro-Benchmark
    std::cout << "[BENCHMARK 6] RevisedSimplex Engine (Vars = 100, Cons = 50)...\n";
    bharatopt::RevisedSimplex rev_simplex_solver;
    auto t_rev_start = std::chrono::high_resolution_clock::now();
    bharatopt::RevisedSimplexResult rev_res = rev_simplex_solver.solve(simplex_bench_model);
    auto t_rev_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_rev = t_rev_end - t_rev_start;

    std::cout << "[RESULT] RevisedSimplex Execution Time: " << duration_rev.count() << " ms\n";
    std::cout << "[RESULT] Solved in " << rev_res.iterations << " Iterations with Status: "
              << (rev_res.status == bharatopt::RevisedSimplexStatus::OPTIMAL ? "OPTIMAL" : "OTHER") << "\n\n";

    // 7. DualRevisedSimplex Micro-Benchmark
    std::cout << "[BENCHMARK 7] DualRevisedSimplex Engine (Vars = 100, Cons = 50)...\n";
    bharatopt::LPModel dual_bench_model("dual_bench");
    dual_bench_model.set_sense(bharatopt::ObjectiveSense::MINIMIZE);
    std::vector<bharatopt::index_t> d_vars;
    for (int j = 0; j < 100; ++j) {
        d_vars.push_back(dual_bench_model.add_variable("x_" + std::to_string(j), 0.0, 100.0, static_cast<bharatopt::real_t>((j % 7) + 1)));
    }
    for (int i = 0; i < 50; ++i) {
        std::vector<std::pair<bharatopt::index_t, bharatopt::real_t>> terms;
        for (int k = 0; k < 5; ++k) {
            terms.push_back({d_vars[(i * 3 + k) % 100], static_cast<bharatopt::real_t>(k + 1)});
        }
        dual_bench_model.add_constraint("c_" + std::to_string(i), terms, bharatopt::ConstraintSense::GREATER_EQUAL, 10.0 + i);
    }

    bharatopt::DualRevisedSimplex dual_simplex_solver;
    auto t_dual_start = std::chrono::high_resolution_clock::now();
    bharatopt::DualRevisedSimplexResult dual_res = dual_simplex_solver.solve(dual_bench_model);
    auto t_dual_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_dual = t_dual_end - t_dual_start;

    std::cout << "[RESULT] DualRevisedSimplex Execution Time: " << duration_dual.count() << " ms\n";
    std::cout << "[RESULT] Solved in " << dual_res.iterations << " Iterations with Status: "
              << (dual_res.status == bharatopt::DualRevisedSimplexStatus::OPTIMAL ? "OPTIMAL" : "OTHER") << "\n\n";

    // 8. Full Refactorisation vs Incremental Basis Updates Benchmark
    std::cout << "[BENCHMARK 8] Full Refactorisation VS Incremental Basis Updates (Vars = 100, Cons = 50)...\n";
    bharatopt::RevisedSimplexOptions opts_full;
    opts_full.enable_incremental_updates = false;
    bharatopt::RevisedSimplex solver_full(opts_full);
    auto t_full_start = std::chrono::high_resolution_clock::now();
    bharatopt::RevisedSimplexResult res_full = solver_full.solve(simplex_bench_model);
    auto t_full_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_full = t_full_end - t_full_start;

    bharatopt::RevisedSimplexOptions opts_inc;
    opts_inc.enable_incremental_updates = true;
    bharatopt::RevisedSimplex solver_inc(opts_inc);
    auto t_inc_start = std::chrono::high_resolution_clock::now();
    bharatopt::RevisedSimplexResult res_inc = solver_inc.solve(simplex_bench_model);
    auto t_inc_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_inc = t_inc_end - t_inc_start;

    std::cout << "[RESULT] Full Refactorisation Time: " << duration_full.count() << " ms (" << res_full.iterations << " iterations)\n";
    std::cout << "[RESULT] Incremental Updates Time: " << duration_inc.count() << " ms (" << res_inc.iterations << " iterations)\n\n";

    // 9. GPU Foundation SpMV & Vector Operations Benchmark
    std::cout << "[BENCHMARK 9] GPU Foundation SpMV & Vector Primitives (Rows = 5,000, Cols = 5,000, NNZ = 50,000)...\n";
    bharatopt::COOMatrix gpu_coo(5000, 5000);
    for (int i = 0; i < 5000; ++i) {
        for (int k = 0; k < 10; ++k) {
            gpu_coo.add_entry(i, (i * 13 + k * 101) % 5000, static_cast<bharatopt::real_t>(k + 1));
        }
    }
    bharatopt::CSRMatrix bench_csr = bharatopt::CSRMatrix::from_coo(gpu_coo);
    std::vector<bharatopt::real_t> bench_x(5000, 1.0);
    std::vector<bharatopt::real_t> bench_gpu_y;
    bharatopt::real_t max_err = 0.0;
    bharatopt::GpuTimingResult timing;

    try {
        bharatopt::GpuBackend::instance().initialize();
        bharatopt::GpuBackend::instance().spmv_with_cpu_comparison(bench_csr, bench_x, bench_gpu_y, max_err, &timing);

        std::cout << "[RESULT] Host-to-Device Transfer Time: " << timing.h2d_time_ms << " ms\n";
        std::cout << "[RESULT] Kernel Execution Time: " << timing.kernel_time_ms << " ms\n";
        std::cout << "[RESULT] Device-to-Host Transfer Time: " << timing.d2h_time_ms << " ms\n";
        std::cout << "[RESULT] Total End-to-End Time: " << timing.total_gpu_time_ms << " ms\n";
        std::cout << "[RESULT] CPU Reference Time: " << timing.cpu_reference_time_ms << " ms\n";
        std::cout << "[RESULT] Max Numerical Error: " << max_err << "\n\n";
    } catch (const std::exception& e) {
        std::cout << "[WARN] GPU Benchmark skipped: " << e.what() << "\n\n";
    } catch (...) {
        std::cout << "[WARN] GPU Benchmark skipped due to hardware environment.\n\n";
    }

    // 10. PDHG / PDLP-Style First-Order LP Solver Micro-Benchmark
    std::cout << "[BENCHMARK 10] First-Order LP Solver Engine (PDHG/PDLP-Style) (Vars = 100, Cons = 50)...\n";
    bharatopt::FirstOrderLPSolver fo_solver;
    auto t_fo_start = std::chrono::high_resolution_clock::now();
    bharatopt::FirstOrderSolverResult fo_res = fo_solver.solve(simplex_bench_model);
    auto t_fo_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration_fo = t_fo_end - t_fo_start;

    std::cout << "[RESULT] First-Order LP Solver Time: " << duration_fo.count() << " ms\n";
    std::cout << "[RESULT] Solved in " << fo_res.stats.iterations << " Iterations with Status: "
              << (fo_res.status == bharatopt::FirstOrderSolverStatus::OPTIMAL ? "OPTIMAL" : "OTHER") << "\n";
    std::cout << "[RESULT] Final Primal Objective: " << fo_res.objective_value << "\n";
    std::cout << "[RESULT] Final Constraint Violation: " << fo_res.stats.constraint_violation << "\n\n";

    // 11. Phase 13 Comprehensive Workload Characterisation Suite
    std::cout << "[BENCHMARK 11] Phase 13 LP Workload Characterisation Suite (Categories A-G)...\n";
    bharatopt::BenchmarkSuite suite;
    suite.add_default_workload_matrix();

    std::vector<bharatopt::BenchmarkResultRecord> results = suite.run_suite({
        bharatopt::BenchmarkSolverType::REVISED_SIMPLEX,
        bharatopt::BenchmarkSolverType::DUAL_REVISED_SIMPLEX,
        bharatopt::BenchmarkSolverType::CPU_FIRST_ORDER,
        bharatopt::BenchmarkSolverType::GPU_FIRST_ORDER
    });

    bharatopt::BenchmarkReporter::print_summary_table(results);

    // Export Machine-Readable Results
    #ifdef _WIN32
        system("mkdir benchmarks\\results 2>nul");
    #else
        system("mkdir -p benchmarks/results");
    #endif

    std::string csv_path = "benchmarks/results/phase_13_results.csv";
    std::string json_path = "benchmarks/results/phase_13_results.json";

    if (bharatopt::BenchmarkReporter::export_csv(results, csv_path)) {
        std::cout << "[INFO] Benchmark CSV results exported to: " << csv_path << "\n";
    }
    if (bharatopt::BenchmarkReporter::export_json(results, json_path)) {
        std::cout << "[INFO] Benchmark JSON results exported to: " << json_path << "\n";
    }

    // 12. Phase 14 Adaptive Cost Estimator Evaluation
    std::cout << "\n[BENCHMARK 12] Phase 14 Adaptive CPU/GPU Cost Estimator Evaluation...\n";
    std::vector<bharatopt::PredictionEvaluationRecord> pred_records;
    bharatopt::CostEstimatorMetrics metrics = bharatopt::CostEstimatorEvaluator::evaluate(results, pred_records);

    std::cout << "[RESULT] Evaluated Benchmark Instances : " << metrics.evaluated_instances << "\n";
    std::cout << "[RESULT] Mean Absolute Error (MAE)      : " << metrics.mae << " ms\n";
    std::cout << "[RESULT] Root Mean Square Error (RMSE)  : " << metrics.rmse << " ms\n";
    std::cout << "[RESULT] Median Absolute Error          : " << metrics.median_absolute_error << " ms\n";
    std::cout << "[RESULT] Ranking Agreement Percentage   : " << metrics.ranking_agreement_pct << " %\n";
    std::cout << "[RESULT] Estimator Overhead Time        : " << metrics.avg_estimator_overhead_ms << " ms\n";

    std::string pred_csv_path = "benchmarks/results/phase_14_predictions.csv";
    std::string pred_json_path = "benchmarks/results/phase_14_predictions.json";

    if (bharatopt::CostEstimatorEvaluator::export_prediction_report_csv(pred_records, pred_csv_path)) {
        std::cout << "[INFO] Prediction CSV report exported to: " << pred_csv_path << "\n";
    }
    if (bharatopt::CostEstimatorEvaluator::export_prediction_report_json(pred_records, pred_json_path)) {
        std::cout << "[INFO] Prediction JSON report exported to: " << pred_json_path << "\n";
    }

    // 13. Phase 15 Adaptive CPU/GPU Execution Router Evaluation
    std::cout << "\n[BENCHMARK 13] Phase 15 Adaptive CPU/GPU Execution Router Evaluation...\n";
    std::vector<bharatopt::RoutingEvaluationRecord> routing_records;
    bharatopt::RouterEvaluationMetrics router_metrics = bharatopt::RouterEvaluator::evaluate(results, routing_records);

    std::cout << "[RESULT] Evaluated Workload Instances   : " << router_metrics.total_evaluated_instances << "\n";
    std::cout << "[RESULT] Selection Agreement Percentage : " << router_metrics.selection_agreement_pct << " %\n";
    std::cout << "[RESULT] Mean Regret                    : " << router_metrics.mean_regret_ms << " ms\n";
    std::cout << "[RESULT] Max Regret                     : " << router_metrics.max_regret_ms << " ms\n";
    std::cout << "[RESULT] Average Routing Overhead       : " << router_metrics.avg_routing_overhead_ms << " ms\n";
    std::cout << "[RESULT] Native GPU Selections          : " << router_metrics.native_gpu_selections << "\n";
    std::cout << "[RESULT] CPU Selections                 : " << router_metrics.cpu_selections << "\n";
    std::cout << "[RESULT] Fallback Events                : " << router_metrics.fallback_events << "\n";

    std::string route_csv_path = "benchmarks/results/phase_15_routing.csv";
    std::string route_json_path = "benchmarks/results/phase_15_routing.json";

    if (bharatopt::RouterEvaluator::export_routing_report_csv(routing_records, route_csv_path)) {
        std::cout << "[INFO] Routing decision CSV report exported to: " << route_csv_path << "\n";
    }
    if (bharatopt::RouterEvaluator::export_routing_report_json(routing_records, route_json_path)) {
        std::cout << "[INFO] Routing decision JSON report exported to: " << route_json_path << "\n";
    }

    // 14. Phase 18 MILP Benchmark Matrix & B&B Workload Characterisation
    std::cout << "\n[BENCHMARK 14] Phase 18 MILP Benchmark Matrix & B&B Workload Characterisation...\n";
    bharatopt::MilpBenchmarkSuite milp_suite;
    milp_suite.add_default_milp_matrix();

    std::vector<bharatopt::MilpBenchmarkResultRecord> milp_results = milp_suite.run_suite();
    bharatopt::BenchmarkReporter::print_milp_summary_table(milp_results);

    std::string milp_csv_path = "benchmarks/results/phase_18_milp_benchmark.csv";
    std::string milp_json_path = "benchmarks/results/phase_18_milp_benchmark.json";

    if (bharatopt::BenchmarkReporter::export_milp_csv(milp_results, milp_csv_path)) {
        std::cout << "[INFO] MILP Benchmark CSV report exported to: " << milp_csv_path << "\n";
    }
    if (bharatopt::BenchmarkReporter::export_milp_json(milp_results, milp_json_path)) {
        std::cout << "[INFO] MILP Benchmark JSON report exported to: " << milp_json_path << "\n";
    }

    // Also export root-level copies for artifact requirements
    bharatopt::BenchmarkReporter::export_milp_csv(milp_results, "phase-18-milp-benchmark.csv");
    bharatopt::BenchmarkReporter::export_milp_json(milp_results, "phase-18-milp-benchmark.json");

    // 15. Phase 19 MILP LP Warm Starts & Incremental Node Re-Optimization
    std::cout << "\n[BENCHMARK 15] Phase 19 MILP LP Warm Starts & Incremental Node Re-Optimization...\n";
    std::vector<bharatopt::MilpWarmStartResultRecord> warm_results = milp_suite.run_warm_start_suite();
    bharatopt::BenchmarkReporter::print_warm_start_summary_table(warm_results);

    std::string warm_csv_path = "benchmarks/results/phase_19_warm_start.csv";
    std::string warm_json_path = "benchmarks/results/phase_19_warm_start.json";

    if (bharatopt::BenchmarkReporter::export_warm_start_csv(warm_results, warm_csv_path)) {
        std::cout << "[INFO] Phase 19 Warm Start CSV report exported to: " << warm_csv_path << "\n";
    }
    if (bharatopt::BenchmarkReporter::export_warm_start_json(warm_results, warm_json_path)) {
        std::cout << "[INFO] Phase 19 Warm Start JSON report exported to: " << warm_json_path << "\n";
    }

    // Export root-level copies as well
    bharatopt::BenchmarkReporter::export_warm_start_csv(warm_results, "phase-19-warm-start.csv");
    bharatopt::BenchmarkReporter::export_warm_start_json(warm_results, "phase-19-warm-start.json");

    } catch (const std::exception& e) {
        std::cout << "[ERROR] Benchmark Suite Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "[ERROR] Benchmark Suite Unknown Exception" << std::endl;
        return 1;
    }

    return 0;
}
