#include <bharatopt/scalability_test.hpp>
#include "test_harness.hpp"
#include <fstream>
#include <cstdio>

namespace bharatopt {

TEST_CASE(Phase23_ScalabilityWorkloadGeneration) {
    ScalabilityResultRecord rec = ScalabilityHarness::run_scalability_experiment(100, 50, 5, 42, 8192.0);
    EXPECT_EQ(rec.n, 100);
    EXPECT_EQ(rec.m, 50);
    EXPECT_EQ(rec.nnz, 500);
    EXPECT_TRUE(rec.generation_time_ms >= 0.0);
    EXPECT_TRUE(rec.construction_time_ms >= 0.0);
}

TEST_CASE(Phase23_SparseNnzCorrectness) {
    ScalabilityResultRecord rec = ScalabilityHarness::run_scalability_experiment(1000, 500, 5, 42, 8192.0);
    EXPECT_EQ(rec.nnz, 5000);
}

TEST_CASE(Phase23_DimensionMetadataCorrectness) {
    ScalabilityResultRecord rec = ScalabilityHarness::run_scalability_experiment(500, 250, 4, 42, 8192.0);
    EXPECT_EQ(rec.experiment_id, "SCALABILITY_500x250");
    EXPECT_EQ(rec.representation, "CSC");
}

TEST_CASE(Phase23_DensityCalculation) {
    ScalabilityResultRecord rec = ScalabilityHarness::run_scalability_experiment(100, 100, 10, 42, 8192.0);
    real_t expected_density = 1000.0 / (100.0 * 100.0); // 0.10
    EXPECT_NEAR(rec.density, expected_density, 1e-6);
}

TEST_CASE(Phase23_MemoryTelemetryValidity) {
    ScalabilityResultRecord rec = ScalabilityHarness::run_scalability_experiment(1000, 500, 5, 42, 8192.0);
    EXPECT_TRUE(rec.matrix_memory_mb > 0.0);
    EXPECT_TRUE(rec.vector_memory_mb > 0.0);
    EXPECT_TRUE(rec.total_memory_mb > 0.0);
}

TEST_CASE(Phase23_CsvSchemaExport) {
    ScalabilityResultRecord rec = ScalabilityHarness::run_scalability_experiment(100, 50, 5, 42, 8192.0);
    std::vector<ScalabilityResultRecord> records = {rec};

    std::string csv_path = "phase-23-scalability.csv";
    bool ok = ScalabilityReporter::export_csv(records, csv_path);
    EXPECT_TRUE(ok);

    std::ifstream file(csv_path);
    EXPECT_TRUE(file.is_open());
    std::string line;
    std::getline(file, line);
    EXPECT_TRUE(line.find("experiment_id") != std::string::npos);
    EXPECT_TRUE(line.find("matrix_memory_mb") != std::string::npos);
}

TEST_CASE(Phase23_JsonSchemaExport) {
    ScalabilityResultRecord rec = ScalabilityHarness::run_scalability_experiment(100, 50, 5, 42, 8192.0);
    std::vector<ScalabilityResultRecord> records = {rec};

    std::string json_path = "phase-23-scalability.json";
    bool ok = ScalabilityReporter::export_json(records, json_path);
    EXPECT_TRUE(ok);

    std::ifstream file(json_path);
    EXPECT_TRUE(file.is_open());
    std::string line;
    std::getline(file, line);
    EXPECT_TRUE(line.find("[") != std::string::npos);
}

TEST_CASE(Phase23_GracefulResourceLimitHandling) {
    // Set max_ram_mb to a very tiny value (0.0001 MB) so estimated memory exceeds limit
    ScalabilityResultRecord rec = ScalabilityHarness::run_scalability_experiment(10000, 5000, 5, 42, 0.0001);
    EXPECT_EQ(rec.execution_status, "SKIPPED_RESOURCE_LIMIT");
    EXPECT_EQ(rec.solver_status, "SKIPPED");
    EXPECT_FALSE(rec.failure_reason.empty());
}

TEST_CASE(Phase23_GpuUnavailableHandling) {
    ScalabilityResultRecord rec = ScalabilityHarness::run_scalability_experiment(100, 50, 5, 42, 8192.0);
    if (!rec.gpu_available) {
        EXPECT_FALSE(rec.native_cuda);
        EXPECT_EQ(rec.gpu_name, "NOT_AVAILABLE");
    }
}

TEST_CASE(Phase23_GpuFallbackClassification) {
    ScalabilityResultRecord rec = ScalabilityHarness::run_scalability_experiment(100, 50, 5, 42, 8192.0);
    if (!rec.gpu_available) {
        EXPECT_EQ(rec.execution_status, "CPU_FALLBACK");
    }
}

TEST_CASE(Phase23_DeterministicWorkloadGeneration) {
    ScalabilityResultRecord rec1 = ScalabilityHarness::run_scalability_experiment(500, 250, 5, 123, 8192.0);
    ScalabilityResultRecord rec2 = ScalabilityHarness::run_scalability_experiment(500, 250, 5, 123, 8192.0);

    EXPECT_EQ(rec1.nnz, rec2.nnz);
    EXPECT_NEAR(rec1.total_memory_mb, rec2.total_memory_mb, 1e-6);
}

TEST_CASE(Phase23_CleanupAndStability) {
    for (int i = 0; i < 5; i++) {
        ScalabilityResultRecord rec = ScalabilityHarness::run_scalability_experiment(200, 100, 5, 42 + i, 8192.0);
        EXPECT_EQ(rec.solver_status, "CONSTRUCTED");
    }
}

} // namespace bharatopt
