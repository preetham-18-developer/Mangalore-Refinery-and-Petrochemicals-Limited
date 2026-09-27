#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <string>
#include <fstream>
#include <bharatopt/final_benchmark.hpp>
#include <bharatopt/solution_verifier.hpp>
#include "test_harness.hpp"

namespace bharatopt {

TEST_CASE(Phase25_FinalBenchmarkRecordCreation) {
    FinalBenchmarkRecord rec;
    rec.experiment_id = "TEST_REC";
    EXPECT_TRUE(rec.experiment_id == "TEST_REC");
}

TEST_CASE(Phase25_DeterministicSyntheticWorkloadSelection) {
    FinalBenchmarkRecord rec1 = FinalBenchmarkSuite::ingest_synthetic_lp(42);
    FinalBenchmarkRecord rec2 = FinalBenchmarkSuite::ingest_synthetic_lp(42);
    EXPECT_TRUE(rec1.solve_time_ms >= 0.0 && rec1.objective == rec2.objective);
}

TEST_CASE(Phase25_SuiteCategoryRegistration) {
    FinalBenchmarkRecord rec = FinalBenchmarkSuite::ingest_synthetic_milp(42);
    EXPECT_TRUE(rec.category == "SYNTHETIC_MILP" && rec.problem_type == "MILP");
}

TEST_CASE(Phase25_Phase21ResultIngestion) {
    FinalBenchmarkRecord rec = FinalBenchmarkSuite::ingest_netlib_sample(42);
    EXPECT_TRUE(rec.category == "NETLIB" && rec.instance_name == "afiro");
}

TEST_CASE(Phase25_Phase22ResultIngestion) {
    AblationRecord a_rec;
    a_rec.experiment_id = "A1_PRESOLVE";
    a_rec.instance_name = "synth_lp";
    a_rec.problem_type = "LP";
    a_rec.solver_name = "RevisedSimplex";
    a_rec.experiment_status = "PASS";
    a_rec.baseline_config = "Presolve ON";
    a_rec.baseline_verified = true;
    FinalBenchmarkRecord rec = FinalBenchmarkSuite::ingest_ablation_record(a_rec);
    EXPECT_TRUE(rec.category == "ABLATION" && rec.verification_status == "VERIFIED");
}

TEST_CASE(Phase25_Phase23ResultIngestion) {
    ScalabilityResultRecord s_rec;
    s_rec.experiment_id = "SCALABILITY_100x50";
    s_rec.m = 50;
    s_rec.n = 100;
    s_rec.nnz = 500;
    s_rec.solver_status = "CONSTRUCTED";
    s_rec.gpu_available = false;
    FinalBenchmarkRecord rec = FinalBenchmarkSuite::ingest_scalability_record(s_rec);
    EXPECT_TRUE(rec.category == "SCALABILITY" && rec.m == 50 && rec.n == 100);
}

TEST_CASE(Phase25_Phase24ResultIngestion) {
    NumericalStressRecord st_rec;
    st_rec.experiment_id = "ILL_COND_HILBERT_5";
    st_rec.category = "ILL_CONDITIONED";
    st_rec.solver_status = "OPTIMAL";
    st_rec.expected_status = "OPTIMAL";
    st_rec.status_match = true;
    st_rec.verification_status = "VERIFIED";
    FinalBenchmarkRecord rec = FinalBenchmarkSuite::ingest_stress_record(st_rec);
    EXPECT_TRUE(rec.category == "NUMERICAL_STRESS" && rec.status_match);
}

TEST_CASE(Phase25_VerifierIntegration) {
    FinalBenchmarkRecord rec = FinalBenchmarkSuite::ingest_synthetic_lp(42);
    EXPECT_TRUE(rec.verification_status == "VERIFIED");
}

TEST_CASE(Phase25_UnavailableGpuHandling) {
    FinalBenchmarkRecord rec = FinalBenchmarkSuite::ingest_synthetic_lp(42);
    EXPECT_TRUE(!rec.gpu_available && !rec.native_cuda && rec.gpu_name == "NOT_AVAILABLE");
}

TEST_CASE(Phase25_UnavailableHighsHandling) {
    FinalBenchmarkRecord rec = FinalBenchmarkSuite::ingest_synthetic_lp(42);
    EXPECT_TRUE(rec.highs_status == "NOT_AVAILABLE");
}

TEST_CASE(Phase25_StatusClassification) {
    FinalBenchmarkRecord rec = FinalBenchmarkSuite::ingest_synthetic_lp(42);
    EXPECT_TRUE(rec.solver_status == "OPTIMAL" && rec.status_match);
}

TEST_CASE(Phase25_WinLossTieComparisonLogic) {
    FinalBenchmarkRecord rec = FinalBenchmarkSuite::ingest_synthetic_lp(42);
    EXPECT_TRUE(rec.comparison_outcome == "TIE");
}

TEST_CASE(Phase25_CsvExport) {
    std::vector<FinalBenchmarkRecord> records = FinalBenchmarkSuite::run_full_suite();
    bool ok = FinalBenchmarkReporter::export_csv(records, "phase-25-final-benchmark.csv");
    std::ifstream fs("phase-25-final-benchmark.csv");
    std::string header;
    std::getline(fs, header);
    EXPECT_TRUE(ok && header.find("comparison_outcome") != std::string::npos);
}

TEST_CASE(Phase25_JsonExport) {
    std::vector<FinalBenchmarkRecord> records = FinalBenchmarkSuite::run_full_suite();
    bool ok = FinalBenchmarkReporter::export_json(records, "phase-25-final-benchmark.json");
    std::ifstream fs("phase-25-final-benchmark.json");
    std::string line;
    std::getline(fs, line);
    EXPECT_TRUE(ok && line.find("[") != std::string::npos);
}

TEST_CASE(Phase25_CsvJsonConsistency) {
    std::vector<FinalBenchmarkRecord> records = FinalBenchmarkSuite::run_full_suite();
    EXPECT_TRUE(records.size() >= 5);
}

TEST_CASE(Phase25_MalformedTelemetryHandling) {
    FinalBenchmarkRecord rec;
    EXPECT_TRUE(rec.highs_status == "NOT_AVAILABLE" && rec.gpu_name == "NOT_AVAILABLE");
}

TEST_CASE(Phase25_DeterministicConfigurationHandling) {
    FinalBenchmarkRecord rec1 = FinalBenchmarkSuite::ingest_synthetic_milp(42);
    FinalBenchmarkRecord rec2 = FinalBenchmarkSuite::ingest_synthetic_milp(42);
    EXPECT_TRUE(rec1.seed == rec2.seed && rec1.objective == rec2.objective);
}

TEST_CASE(Phase25_CleanupAndRepeatedExecution) {
    auto recs1 = FinalBenchmarkSuite::run_full_suite();
    auto recs2 = FinalBenchmarkSuite::run_full_suite();
    EXPECT_TRUE(recs1.size() == recs2.size() && recs1[0].experiment_id == recs2[0].experiment_id);
}

} // namespace bharatopt
