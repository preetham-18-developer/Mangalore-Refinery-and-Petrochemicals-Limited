#include <bharatopt/ablation_study.hpp>
#include <bharatopt/mps_parser.hpp>
#include "test_harness.hpp"
#include <fstream>
#include <cstdio>

namespace bharatopt {

TEST_CASE(Phase22_AblationBaselineReproducibility) {
    LPModel model("ReproModel");
    model.add_variable("x1", 0.0, 10.0, 3.0);
    model.add_variable("x2", 0.0, 10.0, 5.0);
    model.add_constraint("c1", ConstraintSense::LESS_EQUAL, 8.0);
    model.set_coeff(0, 0, 2.0);
    model.set_coeff(0, 1, 1.0);

    AblationRecord rec1 = AblationHarness::run_a1_presolve(model, "ReproModel", 2);
    AblationRecord rec2 = AblationHarness::run_a1_presolve(model, "ReproModel", 2);

    EXPECT_NEAR(rec1.baseline_objective, rec2.baseline_objective, 1e-6);
    EXPECT_EQ(rec1.baseline_iterations, rec2.baseline_iterations);
}

TEST_CASE(Phase22_AblationSingleVariableIsolation) {
    LPModel model("IsoModel");
    model.add_variable("x1", 0.0, 10.0, 3.0);
    model.add_variable("x2", 0.0, 10.0, 5.0);
    model.add_constraint("c1", ConstraintSense::LESS_EQUAL, 8.0);
    model.set_coeff(0, 0, 2.0);
    model.set_coeff(0, 1, 1.0);

    AblationRecord rec = AblationHarness::run_a1_presolve(model, "IsoModel", 2);
    EXPECT_EQ(rec.baseline_config, "Presolve ON");
    EXPECT_EQ(rec.ablated_config, "Presolve OFF");
    EXPECT_EQ(rec.solver_name, "RevisedSimplex");
}

TEST_CASE(Phase22_A1PresolveToggle) {
    MpsParser parser;
    MpsParseResult res = parser.parse_file("benchmarks/netlib/afiro.mps");
    EXPECT_EQ(res.status, MpsParseStatus::SUCCESS);

    AblationRecord rec = AblationHarness::run_a1_presolve(res.model, "afiro", 2);
    EXPECT_EQ(rec.experiment_id, "A1_PRESOLVE");
    EXPECT_EQ(rec.experiment_status, "PASS");
    EXPECT_TRUE(rec.baseline_verified);
    EXPECT_TRUE(rec.ablated_verified);
    EXPECT_NEAR(rec.baseline_objective, rec.ablated_objective, 1e-4);
}

TEST_CASE(Phase22_A2SparseDenseExperimentConfig) {
    LPModel model("SparseDenseModel");
    model.add_variable("x1", 0.0, 10.0, 3.0);
    model.add_variable("x2", 0.0, 10.0, 5.0);
    model.add_constraint("c1", ConstraintSense::LESS_EQUAL, 8.0);
    model.set_coeff(0, 0, 2.0);
    model.set_coeff(0, 1, 1.0);

    AblationRecord rec = AblationHarness::run_a2_sparse(model, "SparseDenseModel", 2);
    EXPECT_EQ(rec.experiment_id, "A2_SPARSE_REPRESENTATION");
    EXPECT_EQ(rec.experiment_status, "PASS");
    EXPECT_TRUE(rec.baseline_verified);
    EXPECT_TRUE(rec.ablated_verified);
    EXPECT_NEAR(rec.baseline_objective, rec.ablated_objective, 1e-4);
}

TEST_CASE(Phase22_A3RoutingModeConfig) {
    LPModel model("RoutingModel");
    model.add_variable("x1", 0.0, 10.0, 3.0);
    model.add_variable("x2", 0.0, 10.0, 5.0);
    model.add_constraint("c1", ConstraintSense::LESS_EQUAL, 8.0);
    model.set_coeff(0, 0, 2.0);
    model.set_coeff(0, 1, 1.0);

    AblationRecord rec = AblationHarness::run_a3_adaptive_routing(model, "RoutingModel", 2);
    EXPECT_EQ(rec.experiment_id, "A3_ADAPTIVE_ROUTING");
    EXPECT_EQ(rec.experiment_status, "PASS");
    EXPECT_TRUE(rec.baseline_verified);
    EXPECT_TRUE(rec.ablated_verified);
}

TEST_CASE(Phase22_A4WarmStartToggle) {
    MpsParser parser;
    MpsParseResult res = parser.parse_file("benchmarks/miplib/blend2.mps");
    EXPECT_EQ(res.status, MpsParseStatus::SUCCESS);

    AblationRecord rec = AblationHarness::run_a4_warm_start(res.model, "blend2", 2);
    EXPECT_EQ(rec.experiment_id, "A4_WARM_START");
    EXPECT_EQ(rec.experiment_status, "PASS");
    EXPECT_TRUE(rec.baseline_verified);
    EXPECT_TRUE(rec.ablated_verified);
    EXPECT_NEAR(rec.baseline_objective, rec.ablated_objective, 1e-4);
}

TEST_CASE(Phase22_A5VerificationToggle) {
    LPModel model("VerifModel");
    model.add_variable("x1", 0.0, 10.0, 3.0);
    model.add_variable("x2", 0.0, 10.0, 5.0);
    model.add_constraint("c1", ConstraintSense::LESS_EQUAL, 8.0);
    model.set_coeff(0, 0, 2.0);
    model.set_coeff(0, 1, 1.0);

    AblationRecord rec = AblationHarness::run_a5_verification_overhead(model, "VerifModel", 2);
    EXPECT_EQ(rec.experiment_id, "A5_VERIFICATION_OVERHEAD");
    EXPECT_EQ(rec.experiment_status, "PASS");
    EXPECT_TRUE(rec.baseline_verified);
    EXPECT_TRUE(rec.baseline_total_time_ms >= rec.baseline_solve_time_ms);
}

TEST_CASE(Phase22_A6CostEstimatorConfig) {
    LPModel model("CostModel");
    model.add_variable("x1", 0.0, 10.0, 3.0);
    model.add_variable("x2", 0.0, 10.0, 5.0);
    model.add_constraint("c1", ConstraintSense::LESS_EQUAL, 8.0);
    model.set_coeff(0, 0, 2.0);
    model.set_coeff(0, 1, 1.0);

    AblationRecord rec = AblationHarness::run_a6_cost_estimator(model, "CostModel", 2);
    EXPECT_EQ(rec.experiment_id, "A6_COST_ESTIMATOR");
    EXPECT_EQ(rec.experiment_status, "PASS");
    EXPECT_TRUE(rec.baseline_verified);
}

TEST_CASE(Phase22_A7BasisPropagationConfig) {
    MpsParser parser;
    MpsParseResult res = parser.parse_file("benchmarks/miplib/blend2.mps");
    EXPECT_EQ(res.status, MpsParseStatus::SUCCESS);

    AblationRecord rec = AblationHarness::run_a7_basis_propagation(res.model, "blend2", 2);
    EXPECT_EQ(rec.experiment_id, "A7_BASIS_PROPAGATION");
    EXPECT_EQ(rec.experiment_status, "PASS");
    EXPECT_TRUE(rec.baseline_verified);
    EXPECT_TRUE(rec.ablated_verified);
}

TEST_CASE(Phase22_AblationResultSchema) {
    AblationRecord r;
    r.experiment_id = "A1_PRESOLVE";
    r.component = "Presolve";
    r.baseline_config = "Presolve ON";
    r.ablated_config = "Presolve OFF";
    r.instance_name = "test_instance";
    r.problem_type = "LP";
    r.solver_name = "RevisedSimplex";
    r.hardware = "CPU";
    r.build_type = "Release";
    r.repetitions = 5;

    EXPECT_EQ(r.experiment_id, "A1_PRESOLVE");
    EXPECT_EQ(r.component, "Presolve");
    EXPECT_EQ(r.hardware, "CPU");
}

TEST_CASE(Phase22_AblationCsvExport) {
    LPModel model("CsvModel");
    model.add_variable("x1", 0.0, 10.0, 3.0);
    model.add_variable("x2", 0.0, 10.0, 5.0);
    model.add_constraint("c1", ConstraintSense::LESS_EQUAL, 8.0);
    model.set_coeff(0, 0, 2.0);
    model.set_coeff(0, 1, 1.0);

    AblationRecord rec = AblationHarness::run_a1_presolve(model, "CsvModel", 2);
    std::vector<AblationRecord> records = {rec};

    std::string csv_path = "phase-22-ablation.csv";
    bool exported = AblationReporter::export_csv(records, csv_path);
    EXPECT_TRUE(exported);

    std::ifstream file(csv_path);
    EXPECT_TRUE(file.is_open());
    std::string line;
    std::getline(file, line); // Header
    EXPECT_TRUE(line.find("experiment_id") != std::string::npos);
}

TEST_CASE(Phase22_AblationJsonExport) {
    LPModel model("JsonModel");
    model.add_variable("x1", 0.0, 10.0, 3.0);
    model.add_variable("x2", 0.0, 10.0, 5.0);
    model.add_constraint("c1", ConstraintSense::LESS_EQUAL, 8.0);
    model.set_coeff(0, 0, 2.0);
    model.set_coeff(0, 1, 1.0);

    AblationRecord rec = AblationHarness::run_a1_presolve(model, "JsonModel", 2);
    std::vector<AblationRecord> records = {rec};

    std::string json_path = "phase-22-ablation.json";
    bool exported = AblationReporter::export_json(records, json_path);
    EXPECT_TRUE(exported);

    std::ifstream file(json_path);
    EXPECT_TRUE(file.is_open());
    std::string line;
    std::getline(file, line);
    EXPECT_TRUE(line.find("[") != std::string::npos);
}

TEST_CASE(Phase22_CorrectnessVerification) {
    LPModel model("CorrectnessModel");
    model.add_variable("x1", 0.0, 10.0, 3.0);
    model.add_variable("x2", 0.0, 10.0, 5.0);
    model.add_constraint("c1", ConstraintSense::LESS_EQUAL, 8.0);
    model.set_coeff(0, 0, 2.0);
    model.set_coeff(0, 1, 1.0);

    AblationRecord rec = AblationHarness::run_a1_presolve(model, "CorrectnessModel", 2);
    EXPECT_TRUE(rec.baseline_verified);
    EXPECT_TRUE(rec.ablated_verified);
    EXPECT_TRUE(rec.max_residual <= 1e-5);
}

TEST_CASE(Phase22_UnsupportedExperimentHandling) {
    LPModel model("LPModelOnly");
    model.add_variable("x1", 0.0, 10.0, 3.0);
    model.add_variable("x2", 0.0, 10.0, 5.0);
    model.add_constraint("c1", ConstraintSense::LESS_EQUAL, 8.0);
    model.set_coeff(0, 0, 2.0);
    model.set_coeff(0, 1, 1.0);

    // Warm start ablation requires MILP, so passing continuous LP returns UNSUPPORTED
    AblationRecord rec = AblationHarness::run_a4_warm_start(model, "LPModelOnly", 2);
    EXPECT_EQ(rec.experiment_status, "UNSUPPORTED");
}

} // namespace bharatopt
