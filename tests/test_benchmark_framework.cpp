#include "test_harness.hpp"
#include <bharatopt/benchmark_framework.hpp>
#include <cstdio>
#include <fstream>

using namespace bharatopt;

TEST_CASE(Benchmark_01_DeterministicGeneration) {
    BenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_det";
    cfg.num_variables = 20;
    cfg.num_constraints = 10;
    cfg.seed = 12345;

    LPModel m1 = BenchmarkGenerator::generate_instance(cfg);
    LPModel m2 = BenchmarkGenerator::generate_instance(cfg);

    EXPECT_EQ(m1.num_variables(), m2.num_variables());
    EXPECT_EQ(m1.num_constraints(), m2.num_constraints());

    for (size_t j = 0; j < m1.num_variables(); ++j) {
        const auto& v1 = m1.get_variable(static_cast<index_t>(j));
        const auto& v2 = m2.get_variable(static_cast<index_t>(j));
        EXPECT_EQ(v1.name, v2.name);
        EXPECT_NEAR(v1.lower_bound, v2.lower_bound, 1e-9);
        EXPECT_NEAR(v1.upper_bound, v2.upper_bound, 1e-9);
        EXPECT_NEAR(v1.obj_coeff, v2.obj_coeff, 1e-9);
    }
}

TEST_CASE(Benchmark_02_FeasibilityGuarantee) {
    BenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_feas";
    cfg.num_variables = 30;
    cfg.num_constraints = 15;
    cfg.seed = 999;

    LPModel model = BenchmarkGenerator::generate_instance(cfg);
    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(Benchmark_03_ResultSerialization) {
    BenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_ser";
    cfg.num_variables = 15;
    cfg.num_constraints = 8;
    cfg.seed = 42;

    BenchmarkResultRecord rec = BenchmarkRunner::run_benchmark(cfg, BenchmarkSolverType::REVISED_SIMPLEX);

    std::vector<BenchmarkResultRecord> records = {rec};
    std::string csv_path = "temp_test_results.csv";
    std::string json_path = "temp_test_results.json";

    EXPECT_TRUE(BenchmarkReporter::export_csv(records, csv_path));
    EXPECT_TRUE(BenchmarkReporter::export_json(records, json_path));

    auto imported = BenchmarkReporter::import_csv(csv_path);
    EXPECT_EQ(imported.size(), static_cast<size_t>(1));
    EXPECT_EQ(imported[0].instance_id, rec.instance_id);
    EXPECT_EQ(imported[0].solver_name, rec.solver_name);
    EXPECT_EQ(imported[0].rows, rec.rows);
    EXPECT_EQ(imported[0].cols, rec.cols);

    std::remove(csv_path.c_str());
    std::remove(json_path.c_str());
}

TEST_CASE(Benchmark_04_TimingValidity) {
    BenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_time";
    cfg.num_variables = 20;
    cfg.num_constraints = 10;

    BenchmarkResultRecord rec = BenchmarkRunner::run_benchmark(cfg, BenchmarkSolverType::DUAL_REVISED_SIMPLEX);

    EXPECT_TRUE(rec.presolve_time_ms >= 0.0);
    EXPECT_TRUE(rec.solve_time_ms >= 0.0);
    EXPECT_NEAR(rec.total_time_ms, rec.presolve_time_ms + rec.solve_time_ms, 1e-5);
}

TEST_CASE(Benchmark_05_IndependentVerificationIntegration) {
    BenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_verify";
    cfg.num_variables = 25;
    cfg.num_constraints = 12;

    BenchmarkResultRecord rec = BenchmarkRunner::run_benchmark(cfg, BenchmarkSolverType::REVISED_SIMPLEX);

    EXPECT_TRUE(rec.verification_passed);
    EXPECT_TRUE(rec.max_residual <= 1e-4);
}

TEST_CASE(Benchmark_06_SparsityDensityCalculation) {
    BenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_density";
    cfg.num_variables = 50;
    cfg.num_constraints = 50;
    cfg.target_density = 0.10;

    BenchmarkResultRecord rec = BenchmarkRunner::run_benchmark(cfg, BenchmarkSolverType::REVISED_SIMPLEX);

    EXPECT_EQ(rec.rows, static_cast<size_t>(50));
    EXPECT_EQ(rec.cols, static_cast<size_t>(50));
    EXPECT_TRUE(rec.nnz > 0);
    EXPECT_NEAR(rec.density, static_cast<real_t>(rec.nnz) / 2500.0, 1e-6);
}

TEST_CASE(Benchmark_07_PresolveImpactMeasurement) {
    BenchmarkInstanceConfig cfg_on;
    cfg_on.instance_id = "test_presolve_on";
    cfg_on.presolve_enabled = true;

    BenchmarkResultRecord rec_on = BenchmarkRunner::run_benchmark(cfg_on, BenchmarkSolverType::REVISED_SIMPLEX);

    BenchmarkInstanceConfig cfg_off;
    cfg_off.instance_id = "test_presolve_off";
    cfg_off.presolve_enabled = false;

    BenchmarkResultRecord rec_off = BenchmarkRunner::run_benchmark(cfg_off, BenchmarkSolverType::REVISED_SIMPLEX);

    EXPECT_TRUE(rec_on.presolve_enabled);
    EXPECT_FALSE(rec_off.presolve_enabled);
    EXPECT_EQ(rec_off.presolve_time_ms, 0.0);
}

TEST_CASE(Benchmark_08_AllSolversExecution) {
    BenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_all_solvers";
    cfg.num_variables = 20;
    cfg.num_constraints = 10;
    cfg.sense = ObjectiveSense::MINIMIZE;
    cfg.seed = 777;

    BenchmarkResultRecord r1 = BenchmarkRunner::run_benchmark(cfg, BenchmarkSolverType::REVISED_SIMPLEX);
    BenchmarkResultRecord r2 = BenchmarkRunner::run_benchmark(cfg, BenchmarkSolverType::DUAL_REVISED_SIMPLEX);
    BenchmarkResultRecord r3 = BenchmarkRunner::run_benchmark(cfg, BenchmarkSolverType::CPU_FIRST_ORDER);
    BenchmarkResultRecord r4 = BenchmarkRunner::run_benchmark(cfg, BenchmarkSolverType::GPU_FIRST_ORDER);

    EXPECT_EQ(r1.status, "OPTIMAL");
    EXPECT_EQ(r2.status, "OPTIMAL");
    EXPECT_EQ(r3.status, "OPTIMAL");
    EXPECT_EQ(r4.status, "OPTIMAL");
}
