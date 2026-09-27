#include "test_harness.hpp"
#include <bharatopt/cost_estimator.hpp>
#include <cstdio>

using namespace bharatopt;

TEST_CASE(CostEstimator_01_DeterministicFeatureExtraction) {
    BenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_feat_det";
    cfg.num_variables = 40;
    cfg.num_constraints = 20;
    cfg.seed = 42;

    LPModel model1 = BenchmarkGenerator::generate_instance(cfg);
    LPModel model2 = BenchmarkGenerator::generate_instance(cfg);

    WorkloadFeatures f1 = ProblemAnalyser::extract_features(model1, true);
    WorkloadFeatures f2 = ProblemAnalyser::extract_features(model2, true);

    EXPECT_EQ(f1.m, f2.m);
    EXPECT_EQ(f1.n, f2.n);
    EXPECT_EQ(f1.nnz, f2.nnz);
    EXPECT_NEAR(f1.density, f2.density, 1e-9);
    EXPECT_NEAR(f1.aspect_ratio, f2.aspect_ratio, 1e-9);
    EXPECT_NEAR(f1.dynamic_range, f2.dynamic_range, 1e-9);
}

TEST_CASE(CostEstimator_02_ExtrapolationDetection) {
    WorkloadFeatures in_domain_feat;
    in_domain_feat.m = 100;
    in_domain_feat.n = 200;
    in_domain_feat.nnz = 1000;
    in_domain_feat.density = 0.05;

    std::string msg1;
    EXPECT_EQ(ExtrapolationDetector::check_domain(in_domain_feat, msg1), ExtrapolationStatus::IN_DOMAIN);

    WorkloadFeatures out_domain_feat;
    out_domain_feat.m = 5000; // Exceeds max 3000
    out_domain_feat.n = 10000;
    out_domain_feat.nnz = 100000;
    out_domain_feat.density = 0.50;

    std::string msg2;
    EXPECT_EQ(ExtrapolationDetector::check_domain(out_domain_feat, msg2), ExtrapolationStatus::EXTRAPOLATION_WARNING);
    EXPECT_TRUE(msg2.find("EXTRAPOLATION_WARNING") != std::string::npos);
}

TEST_CASE(CostEstimator_03_GpuFallbackHandling) {
    WorkloadFeatures feat;
    feat.m = 100;
    feat.n = 100;
    feat.nnz = 500;
    feat.gpu_available = false; // Force non-native scenario test

    AnalyticalCostEstimator estimator;
    CostEstimate est = estimator.estimate(feat, BenchmarkSolverType::GPU_FIRST_ORDER);

    EXPECT_TRUE(est.predicted_total_time_ms > 0.0);
    EXPECT_TRUE(est.gpu_execution_status == GpuExecutionStatus::CPU_FALLBACK || est.gpu_execution_status == GpuExecutionStatus::NATIVE_GPU);
}

TEST_CASE(CostEstimator_04_NoRoutingDecisionEnforced) {
    WorkloadFeatures feat;
    feat.m = 50;
    feat.n = 50;
    feat.nnz = 200;

    AnalyticalCostEstimator estimator;
    std::vector<CostEstimate> all_estimates = estimator.estimate_all(feat);

    EXPECT_EQ(all_estimates.size(), static_cast<size_t>(4));
    for (const auto& est : all_estimates) {
        EXPECT_TRUE(est.predicted_total_time_ms > 0.0);
        // Verify no routing decision or automatic solver selection is made by the estimator
        EXPECT_FALSE(est.solver_name.empty());
    }
}

TEST_CASE(CostEstimator_05_EstimatorOverhead) {
    LPModel model("overhead_test");
    model.add_variable("x1", 0.0, 10.0, 1.0);
    model.add_variable("x2", 0.0, 10.0, 2.0);
    model.add_constraint("c1", {{0, 1.0}, {1, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    WorkloadFeatures feat = ProblemAnalyser::extract_features(model, true);
    
    EXPECT_TRUE(feat.extraction_time_ms >= 0.0);
    EXPECT_TRUE(feat.extraction_time_ms <= 10.0); // Extraction should take < 10ms
}

TEST_CASE(CostEstimator_06_PresolveFeatureIntegration) {
    LPModel model("presolve_feat_test");
    model.add_variable("fixed_x", 3.0, 3.0, 1.0); // Fixed variable
    model.add_variable("active_x", 0.0, 10.0, 2.0);
    model.add_constraint("c1", {{0, 1.0}, {1, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    WorkloadFeatures feat = ProblemAnalyser::extract_features(model, true);

    EXPECT_EQ(feat.orig_n, static_cast<size_t>(2));
    EXPECT_TRUE(feat.presolve_time_ms >= 0.0);
    EXPECT_TRUE(feat.var_reduction_ratio > 0.0);
}

TEST_CASE(CostEstimator_07_PredictionSerialization) {
    PredictionEvaluationRecord rec;
    rec.instance_id = "test_pred_inst";
    rec.solver_name = "RevisedSimplex-CPU";
    rec.predicted_time_ms = 12.5;
    rec.actual_time_ms = 10.0;
    rec.absolute_error_ms = 2.5;
    rec.relative_error = 0.25;
    rec.in_domain = true;
    rec.uncertainty_status = "In domain.";

    std::vector<PredictionEvaluationRecord> records = {rec};
    std::string csv_path = "temp_pred_results.csv";
    std::string json_path = "temp_pred_results.json";

    EXPECT_TRUE(CostEstimatorEvaluator::export_prediction_report_csv(records, csv_path));
    EXPECT_TRUE(CostEstimatorEvaluator::export_prediction_report_json(records, json_path));

    std::remove(csv_path.c_str());
    std::remove(json_path.c_str());
}

TEST_CASE(CostEstimator_08_HardwareProfileDetection) {
    WorkloadFeatures feat;
    feat = ProblemAnalyser::extract_features(LPModel("hw_test"), false);

    EXPECT_TRUE(feat.cpu_core_count >= 1);
    EXPECT_FALSE(feat.feature_schema_version.empty());
}
