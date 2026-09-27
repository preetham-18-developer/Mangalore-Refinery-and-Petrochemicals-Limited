#include "test_harness.hpp"
#include <bharatopt/benchmark_framework.hpp>
#include <bharatopt/branch_and_bound.hpp>
#include <fstream>
#include <cmath>

using namespace bharatopt;

TEST_CASE(MilpBenchmark_01_DeterministicGeneration) {
    MilpBenchmarkInstanceConfig cfg1;
    cfg1.instance_id = "test_gen";
    cfg1.seed = 12345;
    cfg1.num_variables = 20;
    cfg1.num_constraints = 10;
    cfg1.integer_fraction = 0.5;

    MilpBenchmarkInstanceConfig cfg2 = cfg1;

    LPModel m1 = MilpBenchmarkGenerator::generate_instance(cfg1);
    LPModel m2 = MilpBenchmarkGenerator::generate_instance(cfg2);

    EXPECT_EQ(m1.num_variables(), m2.num_variables());
    EXPECT_EQ(m1.num_constraints(), m2.num_constraints());

    for (size_t j = 0; j < m1.num_variables(); ++j) {
        const auto& v1 = m1.get_variable(static_cast<index_t>(j));
        const auto& v2 = m2.get_variable(static_cast<index_t>(j));
        EXPECT_EQ(v1.name, v2.name);
        EXPECT_NEAR(v1.obj_coeff, v2.obj_coeff, 1e-9);
        EXPECT_NEAR(v1.lower_bound, v2.lower_bound, 1e-9);
        EXPECT_NEAR(v1.upper_bound, v2.upper_bound, 1e-9);
        EXPECT_TRUE(v1.type == v2.type);
    }
}

TEST_CASE(MilpBenchmark_02_MetadataCorrectness) {
    MilpBenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_meta";
    cfg.num_variables = 20;
    cfg.num_constraints = 10;
    cfg.integer_fraction = 0.5;
    cfg.binary_ratio = 0.5;

    MilpBenchmarkResultRecord rec = MilpBenchmarkRunner::run_milp_benchmark(cfg);

    EXPECT_EQ(rec.rows, static_cast<size_t>(10));
    EXPECT_EQ(rec.cols, static_cast<size_t>(20));
    EXPECT_TRUE(rec.nnz > 0);
    EXPECT_TRUE(rec.density > 0.0);
    EXPECT_EQ(rec.integer_count + rec.binary_count + rec.continuous_count, static_cast<size_t>(20));
    EXPECT_TRUE(rec.coeff_min <= rec.coeff_max);
}

TEST_CASE(MilpBenchmark_03_LPClassification) {
    MilpBenchmarkInstanceConfig cfg_lp;
    cfg_lp.instance_id = "test_lp_only";
    cfg_lp.integer_fraction = 0.0;

    MilpBenchmarkResultRecord rec_lp = MilpBenchmarkRunner::run_milp_benchmark(cfg_lp);

    EXPECT_EQ(rec_lp.integer_count, static_cast<size_t>(0));
    EXPECT_EQ(rec_lp.binary_count, static_cast<size_t>(0));
    EXPECT_EQ(rec_lp.continuous_count, rec_lp.cols);
}

TEST_CASE(MilpBenchmark_04_TelemetryCompleteness) {
    MilpBenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_telem";
    cfg.num_variables = 15;
    cfg.num_constraints = 8;

    MilpBenchmarkResultRecord rec = MilpBenchmarkRunner::run_milp_benchmark(cfg);

    EXPECT_TRUE(rec.nodes_created > 0);
    EXPECT_TRUE(rec.nodes_processed > 0);
    EXPECT_TRUE(rec.total_solve_time_ms >= 0.0);
    EXPECT_TRUE(rec.root_lp_time_ms >= 0.0);
    EXPECT_FALSE(rec.status.empty());
}

TEST_CASE(MilpBenchmark_05_OptimalResultValidation) {
    MilpBenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_optimal";
    cfg.num_variables = 10;
    cfg.num_constraints = 5;
    cfg.integer_fraction = 0.4;

    MilpBenchmarkResultRecord rec = MilpBenchmarkRunner::run_milp_benchmark(cfg);

    if (rec.status == "OPTIMAL") {
        EXPECT_TRUE(rec.verification_passed);
        EXPECT_TRUE(rec.max_residual <= 1e-4);
    }
}

TEST_CASE(MilpBenchmark_06_LimitReachedHandling) {
    MilpBenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_limit";
    cfg.num_variables = 50;
    cfg.num_constraints = 25;
    cfg.integer_fraction = 0.8;
    cfg.max_nodes = 2; // Strict limit to force node limit cutoff

    MilpBenchmarkResultRecord rec = MilpBenchmarkRunner::run_milp_benchmark(cfg);

    if (rec.nodes_processed >= 2) {
        EXPECT_EQ(rec.status, "LIMIT_REACHED");
        EXPECT_TRUE(rec.node_limit_reached);
    }
}

TEST_CASE(MilpBenchmark_07_BruteForceCrossCheck) {
    LPModel model("test_brute_check");
    model.set_sense(ObjectiveSense::MAXIMIZE);

    auto x = model.add_variable("x", 0.0, 3.0, 2.0, VariableType::INTEGER);
    auto y = model.add_variable("y", 0.0, 3.0, 3.0, VariableType::INTEGER);

    model.add_constraint("c1", {{x, 1.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 4.0);

    BruteForceMilpResult bf_res = BruteForceMilpSolver::solve(model);
    EXPECT_TRUE(bf_res.found_solution);
    EXPECT_NEAR(bf_res.optimum_objective, 11.0, 1e-6); // x=1, y=3 -> 1*2 + 3*3 = 11

    BranchAndBoundEngine bnb_engine;
    BnBResult bnb_res = bnb_engine.solve(model);

    EXPECT_EQ(bnb_res.status, BnBSolverStatus::OPTIMAL);
    EXPECT_NEAR(bnb_res.incumbent.objective_value, bf_res.optimum_objective, 1e-6);
}

TEST_CASE(MilpBenchmark_08_MinimizationHandling) {
    MilpBenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_min";
    cfg.category = MilpBenchmarkCategory::M7_OBJECTIVE_SENSE;
    cfg.sense = ObjectiveSense::MINIMIZE;
    cfg.num_variables = 10;
    cfg.num_constraints = 5;

    MilpBenchmarkResultRecord rec = MilpBenchmarkRunner::run_milp_benchmark(cfg);

    EXPECT_EQ(rec.sense, "MINIMIZE");
    if (rec.status == "OPTIMAL") {
        EXPECT_TRUE(rec.verification_passed);
    }
}

TEST_CASE(MilpBenchmark_09_MaximizationHandling) {
    MilpBenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_max";
    cfg.category = MilpBenchmarkCategory::M7_OBJECTIVE_SENSE;
    cfg.sense = ObjectiveSense::MAXIMIZE;
    cfg.num_variables = 10;
    cfg.num_constraints = 5;

    MilpBenchmarkResultRecord rec = MilpBenchmarkRunner::run_milp_benchmark(cfg);

    EXPECT_EQ(rec.sense, "MAXIMIZE");
    if (rec.status == "OPTIMAL") {
        EXPECT_TRUE(rec.verification_passed);
    }
}

TEST_CASE(MilpBenchmark_10_NumericalVerification) {
    MilpBenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_num_scale";
    cfg.category = MilpBenchmarkCategory::M8_NUMERICAL_SCALE;
    cfg.coeff_min = 0.1;
    cfg.coeff_max = 100.0;
    cfg.num_variables = 10;
    cfg.num_constraints = 5;

    MilpBenchmarkResultRecord rec = MilpBenchmarkRunner::run_milp_benchmark(cfg);

    EXPECT_TRUE(rec.dynamic_range >= 1.0);
    if (rec.status == "OPTIMAL") {
        EXPECT_TRUE(rec.max_residual <= 1e-4);
    }
}

TEST_CASE(MilpBenchmark_11_CsvJsonExportValidation) {
    MilpBenchmarkSuite suite;
    suite.add_default_milp_matrix();

    std::vector<MilpBenchmarkResultRecord> results = suite.run_suite();

    std::string csv_path = "test_milp_out.csv";
    std::string json_path = "test_milp_out.json";

    EXPECT_TRUE(BenchmarkReporter::export_milp_csv(results, csv_path));
    EXPECT_TRUE(BenchmarkReporter::export_milp_json(results, json_path));

    std::ifstream f_csv(csv_path);
    EXPECT_TRUE(f_csv.good());
    f_csv.close();

    std::ifstream f_json(json_path);
    EXPECT_TRUE(f_json.good());
    f_json.close();

    std::remove(csv_path.c_str());
    std::remove(json_path.c_str());
}

TEST_CASE(MilpBenchmark_12_CategoryCoverageValidation) {
    MilpBenchmarkSuite suite;
    suite.add_default_milp_matrix();

    std::vector<MilpBenchmarkResultRecord> results = suite.run_suite();

    bool has_m1 = false, has_m2 = false, has_m3 = false, has_m4 = false, has_m5 = false;
    bool has_m6 = false, has_m7 = false, has_m8 = false, has_m9 = false, has_m10 = false;

    for (const auto& r : results) {
        if (r.category_name == "M1_DimensionScaling") has_m1 = true;
        if (r.category_name == "M2_IntegerDensity") has_m2 = true;
        if (r.category_name == "M3_BinaryVsInteger") has_m3 = true;
        if (r.category_name == "M4_Sparsity") has_m4 = true;
        if (r.category_name == "M5_IntegerFractionality") has_m5 = true;
        if (r.category_name == "M6_TreeGrowth") has_m6 = true;
        if (r.category_name == "M7_ObjectiveSense") has_m7 = true;
        if (r.category_name == "M8_NumericalScale") has_m8 = true;
        if (r.category_name == "M9_HandDerived") has_m9 = true;
        if (r.category_name == "M10_BruteForceCrossCheck") has_m10 = true;
    }

    EXPECT_TRUE(has_m1 && has_m2 && has_m3 && has_m4 && has_m5);
    EXPECT_TRUE(has_m6 && has_m7 && has_m8 && has_m9 && has_m10);
}
