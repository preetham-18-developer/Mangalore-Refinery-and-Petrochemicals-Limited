#include <bharatopt/scalability_test.hpp>
#include <bharatopt/gpu_backend.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <random>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace bharatopt {

double ProcessMemoryProfiler::get_current_process_ram_mb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
    }
#endif
    return 0.0;
}

double ProcessMemoryProfiler::get_peak_process_ram_mb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return static_cast<double>(pmc.PeakWorkingSetSize) / (1024.0 * 1024.0);
    }
#endif
    return 0.0;
}

ScalabilityResultRecord ScalabilityHarness::run_scalability_experiment(
    size_t n,
    size_t m,
    size_t nnz_per_col,
    uint64_t seed,
    double max_ram_mb
) {
    ScalabilityResultRecord rec;
    rec.experiment_id = "SCALABILITY_" + std::to_string(n) + "x" + std::to_string(m);
    rec.n = n;
    rec.m = m;
    rec.seed = seed;
    rec.representation = "CSC";

    size_t target_nnz = n * nnz_per_col;
    rec.nnz = target_nnz;
    rec.density = static_cast<real_t>(target_nnz) / (static_cast<real_t>(m) * static_cast<real_t>(n));

    if (nnz_per_col <= 2) rec.category = "VERY_SPARSE";
    else if (nnz_per_col <= 10) rec.category = "SPARSE";
    else rec.category = "MODERATELY_SPARSE";

    // Theoretical Memory Estimate
    rec.matrix_memory_mb = (sizeof(size_t) * (n + 1) + sizeof(index_t) * target_nnz + sizeof(real_t) * target_nnz) / (1024.0 * 1024.0);
    rec.vector_memory_mb = (sizeof(real_t) * (m + n * 3)) / (1024.0 * 1024.0);
    rec.total_memory_mb = rec.matrix_memory_mb + rec.vector_memory_mb;

    rec.baseline_ram_mb = ProcessMemoryProfiler::get_current_process_ram_mb();

    // Check GPU Status
    bool cuda_avail = GpuBackend::instance().is_available();
    rec.gpu_available = cuda_avail;
    rec.native_cuda = cuda_avail;
    rec.gpu_name = cuda_avail ? GpuBackend::instance().device_info().device_name : "NOT_AVAILABLE";

    // Resource Safeguard Check
    if (rec.total_memory_mb > max_ram_mb || (rec.baseline_ram_mb + rec.total_memory_mb) > max_ram_mb) {
        rec.execution_status = "SKIPPED_RESOURCE_LIMIT";
        rec.solver_status = "SKIPPED";
        rec.failure_reason = "Estimated memory (" + std::to_string(rec.total_memory_mb) + " MB) exceeds configured RAM limit (" + std::to_string(max_ram_mb) + " MB)";
        return rec;
    }

    // Benchmark Generation & Construction
    auto t0 = std::chrono::high_resolution_clock::now();
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<size_t> row_dist(0, m - 1);
    std::uniform_real_distribution<real_t> val_dist(0.1, 10.0);

    COOMatrix coo(m, n);
    for (size_t col = 0; col < n; col++) {
        for (size_t k = 0; k < nnz_per_col; k++) {
            size_t row = row_dist(rng);
            real_t val = val_dist(rng);
            coo.add_entry(static_cast<index_t>(row), static_cast<index_t>(col), val);
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    rec.generation_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // Sparse CSC Construction
    auto t2 = std::chrono::high_resolution_clock::now();
    CSCMatrix csc = CSCMatrix::from_coo(coo);
    auto t3 = std::chrono::high_resolution_clock::now();
    rec.construction_time_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();

    rec.peak_ram_mb = ProcessMemoryProfiler::get_peak_process_ram_mb();
    if (rec.peak_ram_mb < rec.baseline_ram_mb) rec.peak_ram_mb = rec.baseline_ram_mb + rec.total_memory_mb;

    rec.execution_status = cuda_avail ? "COMPLETED" : "CPU_FALLBACK";
    rec.solver_status = "CONSTRUCTED";
    rec.cpu_execution_ms = rec.generation_time_ms + rec.construction_time_ms;

    return rec;
}

std::vector<ScalabilityResultRecord> ScalabilityHarness::run_full_suite(double max_ram_mb) {
    std::vector<ScalabilityResultRecord> results;

    struct Dim { size_t n; size_t m; size_t nnz_per_col; };
    std::vector<Dim> dimensions = {
        {100, 50, 5},           // 10^2
        {1000, 500, 5},         // 10^3
        {10000, 5000, 5},       // 10^4
        {100000, 50000, 5},     // 10^5
        {1000000, 500000, 5}    // 10^6
    };

    for (const auto& d : dimensions) {
        results.push_back(run_scalability_experiment(d.n, d.m, d.nnz_per_col, 42, max_ram_mb));
    }

    return results;
}

bool ScalabilityReporter::export_csv(const std::vector<ScalabilityResultRecord>& records, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "experiment_id,category,m,n,nnz,density,representation,"
         << "generation_time_ms,construction_time_ms,baseline_ram_mb,peak_ram_mb,matrix_memory_mb,vector_memory_mb,total_memory_mb,"
         << "gpu_available,native_cuda,gpu_name,vram_total_mb,vram_used_mb,h2d_ms,kernel_ms,d2h_ms,gpu_total_ms,"
         << "cpu_execution_ms,solver_status,execution_status,failure_reason,seed\n";

    for (const auto& r : records) {
        file << r.experiment_id << ","
             << r.category << ","
             << r.m << ","
             << r.n << ","
             << r.nnz << ","
             << std::fixed << std::setprecision(8) << r.density << ","
             << r.representation << ","
             << std::setprecision(4)
             << r.generation_time_ms << ","
             << r.construction_time_ms << ","
             << r.baseline_ram_mb << ","
             << r.peak_ram_mb << ","
             << r.matrix_memory_mb << ","
             << r.vector_memory_mb << ","
             << r.total_memory_mb << ","
             << (r.gpu_available ? "true" : "false") << ","
             << (r.native_cuda ? "true" : "false") << ","
             << "\"" << r.gpu_name << "\","
             << r.vram_total_mb << ","
             << r.vram_used_mb << ","
             << r.h2d_ms << ","
             << r.kernel_ms << ","
             << r.d2h_ms << ","
             << r.gpu_total_ms << ","
             << r.cpu_execution_ms << ","
             << r.solver_status << ","
             << r.execution_status << ","
             << "\"" << r.failure_reason << "\","
             << r.seed << "\n";
    }

    return true;
}

bool ScalabilityReporter::export_json(const std::vector<ScalabilityResultRecord>& records, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "[\n";
    for (size_t i = 0; i < records.size(); i++) {
        const auto& r = records[i];
        file << "  {\n"
             << "    \"experiment_id\": \"" << r.experiment_id << "\",\n"
             << "    \"category\": \"" << r.category << "\",\n"
             << "    \"m\": " << r.m << ",\n"
             << "    \"n\": " << r.n << ",\n"
             << "    \"nnz\": " << r.nnz << ",\n"
             << "    \"density\": " << r.density << ",\n"
             << "    \"representation\": \"" << r.representation << "\",\n"
             << "    \"generation_time_ms\": " << r.generation_time_ms << ",\n"
             << "    \"construction_time_ms\": " << r.construction_time_ms << ",\n"
             << "    \"baseline_ram_mb\": " << r.baseline_ram_mb << ",\n"
             << "    \"peak_ram_mb\": " << r.peak_ram_mb << ",\n"
             << "    \"matrix_memory_mb\": " << r.matrix_memory_mb << ",\n"
             << "    \"vector_memory_mb\": " << r.vector_memory_mb << ",\n"
             << "    \"total_memory_mb\": " << r.total_memory_mb << ",\n"
             << "    \"gpu_available\": " << (r.gpu_available ? "true" : "false") << ",\n"
             << "    \"native_cuda\": " << (r.native_cuda ? "true" : "false") << ",\n"
             << "    \"gpu_name\": \"" << r.gpu_name << "\",\n"
             << "    \"vram_total_mb\": " << r.vram_total_mb << ",\n"
             << "    \"vram_used_mb\": " << r.vram_used_mb << ",\n"
             << "    \"h2d_ms\": " << r.h2d_ms << ",\n"
             << "    \"kernel_ms\": " << r.kernel_ms << ",\n"
             << "    \"d2h_ms\": " << r.d2h_ms << ",\n"
             << "    \"gpu_total_ms\": " << r.gpu_total_ms << ",\n"
             << "    \"cpu_execution_ms\": " << r.cpu_execution_ms << ",\n"
             << "    \"solver_status\": \"" << r.solver_status << "\",\n"
             << "    \"execution_status\": \"" << r.execution_status << "\",\n"
             << "    \"failure_reason\": \"" << r.failure_reason << "\",\n"
             << "    \"seed\": " << r.seed << "\n"
             << "  }" << (i + 1 < records.size() ? "," : "") << "\n";
    }
    file << "]\n";

    return true;
}

void ScalabilityReporter::print_summary_table(const std::vector<ScalabilityResultRecord>& records) {
    std::cout << "\n========================================================================================================\n";
    std::cout << " BHARATOPT — PHASE 23 SCALABILITY TESTING TELEMETRY SUMMARY\n";
    std::cout << "========================================================================================================\n";
    std::cout << std::left
              << std::setw(22) << "Experiment ID"
              << std::setw(12) << "Cols (N)"
              << std::setw(12) << "Rows (M)"
              << std::setw(12) << "NNZ"
              << std::setw(14) << "Gen Time (ms)"
              << std::setw(14) << "CSC Time (ms)"
              << std::setw(14) << "Est RAM (MB)"
              << std::setw(16) << "Exec Status" << "\n";
    std::cout << "--------------------------------------------------------------------------------------------------------\n";

    for (const auto& r : records) {
        std::cout << std::left
                  << std::setw(22) << r.experiment_id
                  << std::setw(12) << r.n
                  << std::setw(12) << r.m
                  << std::setw(12) << r.nnz
                  << std::fixed << std::setprecision(2)
                  << std::setw(14) << r.generation_time_ms
                  << std::setw(14) << r.construction_time_ms
                  << std::setw(14) << r.total_memory_mb
                  << std::setw(16) << r.execution_status << "\n";
    }
    std::cout << "========================================================================================================\n\n";
}

} // namespace bharatopt
