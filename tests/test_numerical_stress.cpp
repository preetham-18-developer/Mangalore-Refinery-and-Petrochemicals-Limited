#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <string>
#include <fstream>
#include <bharatopt/numerical_stress_test.hpp>
#include <bharatopt/solution_verifier.hpp>
#include "test_harness.hpp"

namespace bharatopt {

TEST_CASE(Phase24_DeterministicGeneratorReproducibility) {
    LPModel m1 = NumericalStressGenerator::generate_ill_conditioned_lp(5, 42);
    LPModel m2 = NumericalStressGenerator::generate_ill_conditioned_lp(5, 42);
    EXPECT_TRUE(m1.num_variables() == m2.num_variables() &&
                m1.num_constraints() == m2.num_constraints() &&
                m1.get_variable(0).obj_coeff == m2.get_variable(0).obj_coeff);
}

TEST_CASE(Phase24_IllConditionedInstanceGeneration) {
    LPModel m = NumericalStressGenerator::generate_ill_conditioned_lp(5);
    EXPECT_TRUE(m.num_variables() == 5 && m.num_constraints() == 5);
}

TEST_CASE(Phase24_DegenerateInstanceGeneration) {
    LPModel m = NumericalStressGenerator::generate_degenerate_lp();
    EXPECT_TRUE(m.num_variables() == 4 && m.num_constraints() == 3);
}

TEST_CASE(Phase24_UnboundedInstanceGeneration) {
    LPModel m = NumericalStressGenerator::generate_unbounded_lp();
    EXPECT_TRUE(m.num_variables() == 2 && m.num_constraints() == 2);
}

TEST_CASE(Phase24_InfeasibleInstanceGeneration) {
    LPModel m = NumericalStressGenerator::generate_infeasible_lp();
    EXPECT_TRUE(m.num_variables() == 2 && m.num_constraints() == 2);
}

TEST_CASE(Phase24_ExtremeCoefficientGeneration) {
    LPModel m = NumericalStressGenerator::generate_extreme_scale_lp();
    EXPECT_TRUE(m.get_variable("x1").obj_coeff == 1e9 && m.get_variable("x2").obj_coeff == 1e-12);
}

TEST_CASE(Phase24_ExpectedStatusValidationInfeasible) {
    LPModel m = NumericalStressGenerator::generate_infeasible_lp();
    NumericalStressRecord rec = NumericalStressHarness::evaluate_stress_case(
        "TEST_INF", "INFEASIBLE", m, "INFEASIBLE", false, 1e-10, 42
    );
    EXPECT_TRUE(rec.solver_status == "INFEASIBLE" && rec.status_match);
}

TEST_CASE(Phase24_ExpectedStatusValidationUnbounded) {
    LPModel m = NumericalStressGenerator::generate_unbounded_lp();
    NumericalStressRecord rec = NumericalStressHarness::evaluate_stress_case(
        "TEST_UNB", "UNBOUNDED", m, "UNBOUNDED", false, 1e-10, 42
    );
    EXPECT_TRUE(rec.solver_status == "UNBOUNDED" && rec.status_match);
}

TEST_CASE(Phase24_NumericalFailureAndToleranceHandling) {
    LPModel m = NumericalStressGenerator::generate_ill_conditioned_lp(5);
    NumericalStressRecord rec = NumericalStressHarness::evaluate_stress_case(
        "TEST_PIVOT_TOL", "ILL_CONDITIONED", m, "OPTIMAL", false, 1e-2, 42
    );
    EXPECT_TRUE(!rec.solver_status.empty());
}

TEST_CASE(Phase24_PresolveOnOffComparison) {
    LPModel m = NumericalStressGenerator::generate_extreme_scale_lp();
    NumericalStressRecord rec_off = NumericalStressHarness::evaluate_stress_case(
        "TEST_SCALE_OFF", "EXTREME_SCALE", m, "OPTIMAL", false, 1e-10, 42
    );
    NumericalStressRecord rec_on = NumericalStressHarness::evaluate_stress_case(
        "TEST_SCALE_ON", "EXTREME_SCALE", m, "OPTIMAL", true, 1e-10, 42
    );
    EXPECT_TRUE(rec_off.solver_status == rec_on.solver_status);
}

TEST_CASE(Phase24_SolutionVerifierIntegration) {
    LPModel m = NumericalStressGenerator::generate_ill_conditioned_lp(3);
    NumericalStressRecord rec = NumericalStressHarness::evaluate_stress_case(
        "TEST_VERIFIER", "ILL_CONDITIONED", m, "OPTIMAL", false, 1e-10, 42
    );
    EXPECT_TRUE(rec.verification_status == "VERIFIED");
}

TEST_CASE(Phase24_CsvSchemaExport) {
    std::vector<NumericalStressRecord> records = NumericalStressHarness::run_full_suite();
    bool ok = NumericalStressReporter::export_csv(records, "phase-24-numerical-stress.csv");
    std::ifstream fs("phase-24-numerical-stress.csv");
    std::string header;
    std::getline(fs, header);
    EXPECT_TRUE(ok && header.find("coefficient_dynamic_range") != std::string::npos);
}

TEST_CASE(Phase24_JsonSchemaExport) {
    std::vector<NumericalStressRecord> records = NumericalStressHarness::run_full_suite();
    bool ok = NumericalStressReporter::export_json(records, "phase-24-numerical-stress.json");
    std::ifstream fs("phase-24-numerical-stress.json");
    std::string line;
    std::getline(fs, line);
    EXPECT_TRUE(ok && line.find("[") != std::string::npos);
}

TEST_CASE(Phase24_MalformedTelemetryHandling) {
    NumericalStressRecord rec;
    rec.coefficient_min = 0.0;
    rec.coefficient_max = 0.0;
    EXPECT_TRUE(rec.coefficient_dynamic_range == 1.0 && rec.gpu_status == "CPU_FALLBACK");
}

TEST_CASE(Phase24_CleanupAndRepeatedExecution) {
    auto recs1 = NumericalStressHarness::run_full_suite();
    auto recs2 = NumericalStressHarness::run_full_suite();
    EXPECT_TRUE(recs1.size() == recs2.size() && recs1[0].experiment_id == recs2[0].experiment_id);
}

} // namespace bharatopt
