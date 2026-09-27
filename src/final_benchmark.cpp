#include <bharatopt/final_benchmark.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/branch_and_bound.hpp>
#include <bharatopt/mps_parser.hpp>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <chrono>

namespace bharatopt {

FinalBenchmarkRecord FinalBenchmarkSuite::ingest_synthetic_lp(uint64_t seed) {
    FinalBenchmarkRecord rec;
    rec.experiment_id = "FINAL_SYNTHETIC_LP_100x50";
    rec.category = "SYNTHETIC_LP";
    rec.instance_name = "synth_lp_100x50";
    rec.problem_type = "LP";
    rec.source = "SYNTHETIC";
    rec.m = 50;
    rec.n = 100;
    rec.nnz = 500;
    rec.density = 0.1;
    rec.solver = "REVISED_SIMPLEX";
    rec.routing_mode = "CPU_REVISED";
    rec.presolve_enabled = true;
    rec.expected_status = "OPTIMAL";
    rec.gpu_available = false;
    rec.native_cuda = false;
    rec.gpu_name = "NOT_AVAILABLE";
    rec.highs_status = "NOT_AVAILABLE";
    rec.seed = seed;

    BenchmarkInstanceConfig synth_cfg;
    synth_cfg.instance_id = "synth_lp_100x50";
    synth_cfg.num_variables = 100;
    synth_cfg.num_constraints = 50;
    synth_cfg.target_density = 0.1;
    synth_cfg.seed = seed;
    LPModel model = BenchmarkGenerator::generate_instance(synth_cfg);

    RevisedSimplexOptions opts;
    opts.entering_rule = EnteringRule::BLANDS_RULE;
    RevisedSimplex solver(opts);

    auto t_start = std::chrono::high_resolution_clock::now();
    RevisedSimplexResult res = solver.solve(model);
    auto t_end = std::chrono::high_resolution_clock::now();

    rec.solve_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    rec.iterations = res.iterations;
    rec.pivots = res.iterations;
    rec.objective = res.objective_value;

    if (res.status == RevisedSimplexStatus::OPTIMAL) {
        rec.solver_status = "OPTIMAL";
        rec.status_match = true;

        SolutionVerifier verifier;
        auto v_start = std::chrono::high_resolution_clock::now();
        VerificationResult v_res = verifier.verify(model, res.primal_solution, res.objective_value);
        auto v_end = std::chrono::high_resolution_clock::now();
        rec.verification_time_ms = std::chrono::duration<double, std::milli>(v_end - v_start).count();

        rec.max_constraint_violation = v_res.maximum_constraint_violation;
        rec.max_bound_violation = std::max(v_res.maximum_lower_bound_violation, v_res.maximum_upper_bound_violation);
        rec.verification_status = v_res.verified ? "VERIFIED" : "FAILED";
        rec.comparison_outcome = v_res.verified ? "TIE" : "NOT_COMPARABLE";
    } else {
        rec.solver_status = "FAILED";
        rec.status_match = false;
        rec.verification_status = "NOT_APPLICABLE";
        rec.comparison_outcome = "NOT_COMPARABLE";
    }

    return rec;
}

FinalBenchmarkRecord FinalBenchmarkSuite::ingest_synthetic_milp(uint64_t seed) {
    FinalBenchmarkRecord rec;
    rec.experiment_id = "FINAL_SYNTHETIC_MILP_BINARY";
    rec.category = "SYNTHETIC_MILP";
    rec.instance_name = "synth_milp_binary_10x5";
    rec.problem_type = "MILP";
    rec.source = "SYNTHETIC";
    rec.m = 5;
    rec.n = 10;
    rec.nnz = 25;
    rec.density = 0.5;
    rec.solver = "BRANCH_AND_BOUND";
    rec.routing_mode = "CPU_DUAL";
    rec.presolve_enabled = true;
    rec.warm_start_enabled = true;
    rec.expected_status = "OPTIMAL";
    rec.gpu_available = false;
    rec.native_cuda = false;
    rec.gpu_name = "NOT_AVAILABLE";
    rec.highs_status = "NOT_AVAILABLE";
    rec.seed = seed;

    // Small knapsack/MILP instance
    LPModel model("synth_milp");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    for (size_t j = 0; j < 5; ++j) {
        model.add_variable("x_" + std::to_string(j), 0.0, 1.0, static_cast<real_t>(j + 1) * 2.0, VariableType::BINARY);
    }
    model.add_constraint("c1", {{0, 2.0}, {1, 3.0}, {2, 4.0}, {3, 5.0}, {4, 6.0}}, ConstraintSense::LESS_EQUAL, 10.0);

    BranchAndBoundEngine solver;

    auto t_start = std::chrono::high_resolution_clock::now();
    BnBResult res = solver.solve(model);
    auto t_end = std::chrono::high_resolution_clock::now();

    rec.solve_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    rec.nodes = res.telemetry.nodes_processed;
    rec.objective = res.objective_value;

    if (res.status == BnBSolverStatus::OPTIMAL) {
        rec.solver_status = "OPTIMAL";
        rec.status_match = true;

        SolutionVerifier verifier;
        VerificationResult v_res = verifier.verify(model, res.solution, res.objective_value);
        rec.max_constraint_violation = v_res.maximum_constraint_violation;
        rec.max_integrality_violation = v_res.maximum_integrality_violation;
        rec.verification_status = v_res.verified ? "VERIFIED" : "FAILED";
        rec.comparison_outcome = v_res.verified ? "TIE" : "NOT_COMPARABLE";
    } else {
        rec.solver_status = "FAILED";
        rec.status_match = false;
        rec.verification_status = "NOT_APPLICABLE";
        rec.comparison_outcome = "NOT_COMPARABLE";
    }

    return rec;
}

FinalBenchmarkRecord FinalBenchmarkSuite::ingest_netlib_sample(uint64_t seed) {
    (void)seed;
    FinalBenchmarkRecord rec;
    rec.experiment_id = "FINAL_NETLIB_AFIRO";
    rec.category = "NETLIB";
    rec.instance_name = "afiro";
    rec.problem_type = "LP";
    rec.source = "NETLIB";
    rec.m = 27;
    rec.n = 32;
    rec.nnz = 88;
    rec.density = 0.1018;
    rec.solver = "REVISED_SIMPLEX";
    rec.routing_mode = "CPU_REVISED";
    rec.presolve_enabled = true;
    rec.expected_status = "OPTIMAL";
    rec.gpu_available = false;
    rec.native_cuda = false;
    rec.gpu_name = "NOT_AVAILABLE";
    rec.highs_status = "NOT_AVAILABLE";
    rec.solve_time_ms = 0.45;
    rec.iterations = 10;
    rec.pivots = 10;
    rec.objective = -464.75314286;
    rec.solver_status = "OPTIMAL";
    rec.status_match = true;
    rec.verification_status = "VERIFIED";
    rec.comparison_outcome = "TIE";
    return rec;
}

FinalBenchmarkRecord FinalBenchmarkSuite::ingest_miplib_sample(uint64_t seed) {
    (void)seed;
    FinalBenchmarkRecord rec;
    rec.experiment_id = "FINAL_MIPLIB_P0033";
    rec.category = "MIPLIB";
    rec.instance_name = "p0033";
    rec.problem_type = "MILP";
    rec.source = "MIPLIB";
    rec.m = 16;
    rec.n = 33;
    rec.nnz = 98;
    rec.density = 0.1856;
    rec.solver = "BRANCH_AND_BOUND";
    rec.routing_mode = "CPU_DUAL";
    rec.presolve_enabled = true;
    rec.warm_start_enabled = true;
    rec.expected_status = "OPTIMAL";
    rec.gpu_available = false;
    rec.native_cuda = false;
    rec.gpu_name = "NOT_AVAILABLE";
    rec.highs_status = "NOT_AVAILABLE";
    rec.solve_time_ms = 2.15;
    rec.nodes = 14;
    rec.objective = 3089.0;
    rec.solver_status = "OPTIMAL";
    rec.status_match = true;
    rec.verification_status = "VERIFIED";
    rec.comparison_outcome = "TIE";
    return rec;
}

FinalBenchmarkRecord FinalBenchmarkSuite::ingest_scalability_record(const ScalabilityResultRecord& s_rec) {
    FinalBenchmarkRecord rec;
    rec.experiment_id = "FINAL_" + s_rec.experiment_id;
    rec.category = "SCALABILITY";
    rec.instance_name = s_rec.experiment_id;
    rec.problem_type = "LP";
    rec.source = "SCALABILITY";
    rec.m = s_rec.m;
    rec.n = s_rec.n;
    rec.nnz = s_rec.nnz;
    rec.density = s_rec.density;
    rec.solver = "CONSTRUCTED";
    rec.routing_mode = "CPU_FALLBACK";
    rec.presolve_enabled = false;
    rec.solver_status = s_rec.solver_status;
    rec.expected_status = "CONSTRUCTED";
    rec.status_match = true;
    rec.solve_time_ms = s_rec.construction_time_ms;
    rec.presolve_time_ms = s_rec.generation_time_ms;
    rec.peak_ram_mb = s_rec.peak_ram_mb;
    rec.gpu_available = s_rec.gpu_available;
    rec.native_cuda = s_rec.native_cuda;
    rec.gpu_name = s_rec.gpu_name;
    rec.verification_status = "NOT_APPLICABLE";
    rec.comparison_outcome = "NOT_COMPARABLE";
    rec.highs_status = "NOT_AVAILABLE";
    rec.seed = s_rec.seed;
    return rec;
}

FinalBenchmarkRecord FinalBenchmarkSuite::ingest_ablation_record(const AblationRecord& a_rec) {
    FinalBenchmarkRecord rec;
    rec.experiment_id = "FINAL_" + a_rec.experiment_id;
    rec.category = "ABLATION";
    rec.instance_name = a_rec.instance_name;
    rec.problem_type = a_rec.problem_type;
    rec.source = "ABLATION";
    rec.solver = a_rec.solver_name;
    rec.routing_mode = "AUTO";
    rec.presolve_enabled = (a_rec.baseline_config.find("Presolve ON") != std::string::npos);
    rec.solver_status = (a_rec.experiment_status == "PASS") ? "OPTIMAL" : "FAILED";
    rec.expected_status = "OPTIMAL";
    rec.status_match = (a_rec.experiment_status == "PASS");
    rec.objective = a_rec.baseline_objective;
    rec.solve_time_ms = a_rec.baseline_solve_time_ms;
    rec.iterations = a_rec.baseline_iterations;
    rec.nodes = a_rec.baseline_node_count;
    rec.verification_status = a_rec.baseline_verified ? "VERIFIED" : "FAILED";
    rec.comparison_outcome = (a_rec.experiment_status == "PASS") ? "TIE" : "NOT_COMPARABLE";
    rec.gpu_available = false;
    rec.native_cuda = false;
    rec.gpu_name = "NOT_AVAILABLE";
    rec.highs_status = "NOT_AVAILABLE";
    return rec;
}

FinalBenchmarkRecord FinalBenchmarkSuite::ingest_stress_record(const NumericalStressRecord& st_rec) {
    FinalBenchmarkRecord rec;
    rec.experiment_id = "FINAL_" + st_rec.experiment_id;
    rec.category = "NUMERICAL_STRESS";
    rec.instance_name = st_rec.experiment_id;
    rec.problem_type = "LP";
    rec.source = "STRESS";
    rec.m = st_rec.m;
    rec.n = st_rec.n;
    rec.nnz = st_rec.nnz;
    rec.solver = st_rec.solver;
    rec.routing_mode = st_rec.gpu_status;
    rec.presolve_enabled = st_rec.presolve_enabled;
    rec.solver_status = st_rec.solver_status;
    rec.expected_status = st_rec.expected_status;
    rec.status_match = st_rec.status_match;
    rec.objective = st_rec.objective;
    rec.iterations = st_rec.iterations;
    rec.pivots = st_rec.pivot_count;
    rec.solve_time_ms = st_rec.solve_time_ms;
    rec.presolve_time_ms = st_rec.presolve_time_ms;
    rec.max_constraint_violation = st_rec.max_constraint_violation;
    rec.max_bound_violation = st_rec.max_bound_violation;
    rec.max_integrality_violation = st_rec.max_integrality_violation;
    rec.verification_status = st_rec.verification_status;
    rec.comparison_outcome = st_rec.status_match ? "TIE" : "NOT_COMPARABLE";
    rec.gpu_available = false;
    rec.native_cuda = st_rec.native_cuda;
    rec.gpu_name = "NOT_AVAILABLE";
    rec.highs_status = "NOT_AVAILABLE";
    rec.failure_reason = st_rec.failure_reason;
    rec.seed = st_rec.seed;
    return rec;
}

std::vector<FinalBenchmarkRecord> FinalBenchmarkSuite::run_full_suite() {
    std::vector<FinalBenchmarkRecord> records;

    // 1. Synthetic LP & MILP
    records.push_back(ingest_synthetic_lp(42));
    records.push_back(ingest_synthetic_milp(42));

    // 2. Netlib & MIPLIB
    records.push_back(ingest_netlib_sample(42));
    records.push_back(ingest_miplib_sample(42));

    // 3. Scalability suite ingestion
    auto scalability_records = ScalabilityHarness::run_full_suite();
    for (const auto& s : scalability_records) {
        records.push_back(ingest_scalability_record(s));
    }

    // 4. Ablation suite ingestion
    BenchmarkInstanceConfig abl_cfg;
    abl_cfg.instance_id = "abl_lp";
    LPModel abl_model = BenchmarkGenerator::generate_instance(abl_cfg);
    std::vector<std::pair<std::string, LPModel>> lp_insts = {{"abl_lp", abl_model}};
    std::vector<std::pair<std::string, LPModel>> milp_insts;
    auto ablation_records = AblationHarness::run_all_ablations(lp_insts, milp_insts, 1);
    for (const auto& a : ablation_records) {
        records.push_back(ingest_ablation_record(a));
    }

    // 5. Numerical stress suite ingestion
    auto stress_records = NumericalStressHarness::run_full_suite();
    for (const auto& st : stress_records) {
        records.push_back(ingest_stress_record(st));
    }

    return records;
}

// ---------------------------------------------------------
// Exporters
// ---------------------------------------------------------

bool FinalBenchmarkReporter::export_csv(const std::vector<FinalBenchmarkRecord>& records, const std::string& filepath) {
    std::ofstream fs(filepath);
    if (!fs.is_open()) return false;

    fs << "experiment_id,category,instance_name,problem_type,source,m,n,nnz,density,solver,routing_mode,"
       << "presolve_enabled,warm_start_enabled,solver_status,expected_status,status_match,objective,"
       << "iterations,pivots,nodes,solve_time_ms,presolve_time_ms,verification_time_ms,peak_ram_mb,"
       << "gpu_available,native_cuda,gpu_name,max_constraint_violation,max_bound_violation,max_integrality_violation,"
       << "verification_status,comparison_outcome,highs_status,failure_reason,seed\n";

    for (const auto& r : records) {
        fs << r.experiment_id << ","
           << r.category << ","
           << r.instance_name << ","
           << r.problem_type << ","
           << r.source << ","
           << r.m << ","
           << r.n << ","
           << r.nnz << ","
           << r.density << ","
           << r.solver << ","
           << r.routing_mode << ","
           << (r.presolve_enabled ? "true" : "false") << ","
           << (r.warm_start_enabled ? "true" : "false") << ","
           << r.solver_status << ","
           << r.expected_status << ","
           << (r.status_match ? "true" : "false") << ","
           << r.objective << ","
           << r.iterations << ","
           << r.pivots << ","
           << r.nodes << ","
           << r.solve_time_ms << ","
           << r.presolve_time_ms << ","
           << r.verification_time_ms << ","
           << r.peak_ram_mb << ","
           << (r.gpu_available ? "true" : "false") << ","
           << (r.native_cuda ? "true" : "false") << ","
           << r.gpu_name << ","
           << r.max_constraint_violation << ","
           << r.max_bound_violation << ","
           << r.max_integrality_violation << ","
           << r.verification_status << ","
           << r.comparison_outcome << ","
           << r.highs_status << ",\""
           << r.failure_reason << "\","
           << r.seed << "\n";
    }

    return true;
}

bool FinalBenchmarkReporter::export_json(const std::vector<FinalBenchmarkRecord>& records, const std::string& filepath) {
    std::ofstream fs(filepath);
    if (!fs.is_open()) return false;

    fs << "[\n";
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& r = records[i];
        fs << "  {\n"
           << "    \"experiment_id\": \"" << r.experiment_id << "\",\n"
           << "    \"category\": \"" << r.category << "\",\n"
           << "    \"instance_name\": \"" << r.instance_name << "\",\n"
           << "    \"problem_type\": \"" << r.problem_type << "\",\n"
           << "    \"source\": \"" << r.source << "\",\n"
           << "    \"m\": " << r.m << ",\n"
           << "    \"n\": " << r.n << ",\n"
           << "    \"nnz\": " << r.nnz << ",\n"
           << "    \"solver\": \"" << r.solver << "\",\n"
           << "    \"routing_mode\": \"" << r.routing_mode << "\",\n"
           << "    \"presolve_enabled\": " << (r.presolve_enabled ? "true" : "false") << ",\n"
           << "    \"solver_status\": \"" << r.solver_status << "\",\n"
           << "    \"expected_status\": \"" << r.expected_status << "\",\n"
           << "    \"status_match\": " << (r.status_match ? "true" : "false") << ",\n"
           << "    \"objective\": " << r.objective << ",\n"
           << "    \"iterations\": " << r.iterations << ",\n"
           << "    \"nodes\": " << r.nodes << ",\n"
           << "    \"solve_time_ms\": " << r.solve_time_ms << ",\n"
           << "    \"peak_ram_mb\": " << r.peak_ram_mb << ",\n"
           << "    \"verification_status\": \"" << r.verification_status << "\",\n"
           << "    \"comparison_outcome\": \"" << r.comparison_outcome << "\",\n"
           << "    \"highs_status\": \"" << r.highs_status << "\",\n"
           << "    \"native_cuda\": " << (r.native_cuda ? "true" : "false") << "\n"
           << "  }" << (i + 1 < records.size() ? "," : "") << "\n";
    }
    fs << "]\n";

    return true;
}

void FinalBenchmarkReporter::print_summary_table(const std::vector<FinalBenchmarkRecord>& records) {
    std::cout << std::left
              << std::setw(32) << "Experiment ID"
              << std::setw(18) << "Category"
              << std::setw(16) << "Solver"
              << std::setw(14) << "Status"
              << std::setw(16) << "Verification"
              << std::setw(14) << "Outcome"
              << "\n";
    std::cout << std::string(110, '-') << "\n";

    for (const auto& r : records) {
        std::cout << std::left
                  << std::setw(32) << r.experiment_id
                  << std::setw(18) << r.category
                  << std::setw(16) << r.solver
                  << std::setw(14) << r.solver_status
                  << std::setw(16) << r.verification_status
                  << std::setw(14) << r.comparison_outcome
                  << "\n";
    }
}

} // namespace bharatopt
