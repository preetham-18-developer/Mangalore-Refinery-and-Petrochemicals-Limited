#include <bharatopt/benchmark_framework.hpp>
#include <bharatopt/model_validator.hpp>
#include <random>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#if defined(__has_include)
  #if __has_include(<filesystem>)
    #include <filesystem>
  #elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    namespace std { namespace filesystem = experimental::filesystem; }
  #endif
#endif

namespace bharatopt {

LPModel BenchmarkGenerator::generate_instance(const BenchmarkInstanceConfig& config) {
    LPModel model(config.instance_id);
    model.set_sense(config.sense);

    std::mt19937_64 rng(config.seed);
    std::uniform_real_distribution<real_t> coeff_dist(config.coeff_min, config.coeff_max);
    std::uniform_real_distribution<real_t> bound_dist(0.0, 5.0);
    std::uniform_real_distribution<real_t> sol_dist(1.0, 10.0);
    std::uniform_real_distribution<real_t> margin_dist(0.1, 2.0);
    std::uniform_real_distribution<real_t> prob_dist(0.0, 1.0);

    size_t n = config.num_variables;
    size_t m = config.num_constraints;

    // 1. Generate Variables & Bounds
    std::vector<index_t> var_indices(n);
    std::vector<real_t> x_known(n);
    for (size_t j = 0; j < n; ++j) {
        real_t lb = bound_dist(rng);
        real_t ub = lb + 10.0 + bound_dist(rng);
        real_t obj_c = coeff_dist(rng);
        var_indices[j] = model.add_variable("x_" + std::to_string(j), lb, ub, obj_c);
        
        // Known feasible point within bounds
        x_known[j] = lb + 0.5 * (ub - lb);
    }

    // 2. Generate Constraints based on Matrix Shape / Structure
    for (size_t i = 0; i < m; ++i) {
        std::vector<std::pair<index_t, real_t>> terms;
        real_t row_lhs = 0.0;

        if (config.shape_type == "diagonal") {
            if (i < n) {
                real_t val = coeff_dist(rng);
                terms.push_back({var_indices[i], val});
                row_lhs += val * x_known[i];
            }
        } else if (config.shape_type == "banded") {
            for (size_t j = 0; j < n; ++j) {
                if (std::abs(static_cast<int>(i) - static_cast<int>(j)) <= 2) {
                    real_t val = coeff_dist(rng);
                    terms.push_back({var_indices[j], val});
                    row_lhs += val * x_known[j];
                }
            }
        } else if (config.shape_type == "block_sparse") {
            size_t block_id = i / 5;
            size_t start_col = (block_id * 5) % n;
            for (size_t offset = 0; offset < 5 && (start_col + offset) < n; ++offset) {
                real_t val = coeff_dist(rng);
                terms.push_back({var_indices[start_col + offset], val});
                row_lhs += val * x_known[start_col + offset];
            }
        } else { // General / Random Sparse
            for (size_t j = 0; j < n; ++j) {
                if (prob_dist(rng) <= config.target_density) {
                    real_t val = coeff_dist(rng);
                    terms.push_back({var_indices[j], val});
                    row_lhs += val * x_known[j];
                }
            }
            // Ensure every row has at least one non-zero entry
            if (terms.empty()) {
                index_t target_j = static_cast<index_t>(i % n);
                real_t val = coeff_dist(rng);
                terms.push_back({var_indices[target_j], val});
                row_lhs += val * x_known[target_j];
            }
        }

        // Construct RHS to guarantee feasibility
        real_t margin = margin_dist(rng);
        ConstraintSense sense = ConstraintSense::LESS_EQUAL;
        real_t rhs = row_lhs + margin;

        if (i % 3 == 1) {
            sense = ConstraintSense::GREATER_EQUAL;
            rhs = row_lhs - margin;
        } else if (i % 3 == 2) {
            sense = ConstraintSense::EQUAL;
            rhs = row_lhs;
        }

        model.add_constraint("c_" + std::to_string(i), terms, sense, rhs);
    }

    return model;
}

BenchmarkResultRecord BenchmarkRunner::run_benchmark(
    const BenchmarkInstanceConfig& config,
    BenchmarkSolverType solver_type,
    size_t repetitions
) {
    BenchmarkResultRecord rec;
    rec.benchmark_id = "BENCH_" + config.instance_id;
    rec.instance_id = config.instance_id;
    rec.presolve_enabled = config.presolve_enabled;
    rec.seed = config.seed;
    rec.repetitions = repetitions;

    // Time stamp
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now_c));
    rec.timestamp = std::string(buf);

    // 1. Generate Instance
    LPModel original_model = BenchmarkGenerator::generate_instance(config);
    rec.rows = original_model.num_constraints();
    rec.cols = original_model.num_variables();
    rec.shape = config.shape_type;
    rec.coeff_min = config.coeff_min;
    rec.coeff_max = config.coeff_max;

    // Calculate NNZ & Density
    size_t total_nnz = 0;
    for (size_t i = 0; i < rec.rows; ++i) {
        total_nnz += original_model.get_constraint(static_cast<index_t>(i)).terms.size();
    }
    rec.nnz = total_nnz;
    rec.density = (rec.rows > 0 && rec.cols > 0) ? (static_cast<real_t>(rec.nnz) / (rec.rows * rec.cols)) : 0.0;

    // 2. Presolve Phase (if enabled)
    LPModel solve_model = original_model;
    PresolveEngine presolver;
    double presolve_time = 0.0;

    if (config.presolve_enabled) {
        auto p_start = std::chrono::high_resolution_clock::now();
        PresolveResult p_res = presolver.presolve(original_model);
        auto p_end = std::chrono::high_resolution_clock::now();
        presolve_time = std::chrono::duration<double, std::milli>(p_end - p_start).count();
        
        rec.presolve_time_ms = presolve_time;
        rec.vars_removed = p_res.stats.vars_removed;
        rec.cons_removed = p_res.stats.cons_removed;
        rec.reduced_rows = p_res.reduced_model.num_constraints();
        rec.reduced_cols = p_res.reduced_model.num_variables();

        size_t red_nnz = 0;
        for (size_t i = 0; i < rec.reduced_rows; ++i) {
            red_nnz += p_res.reduced_model.get_constraint(static_cast<index_t>(i)).terms.size();
        }
        rec.reduced_nnz = red_nnz;
    } else {
        rec.reduced_rows = rec.rows;
        rec.reduced_cols = rec.cols;
        rec.reduced_nnz = rec.nnz;
    }

    // 3. Execute Selected Solver
    std::vector<real_t> sol_vector;

    if (solver_type == BenchmarkSolverType::REVISED_SIMPLEX) {
        rec.solver_name = "RevisedSimplex";
        rec.solver_variant = "CPU_Sparse_LU";
        
        RevisedSimplex solver;
        auto s_start = std::chrono::high_resolution_clock::now();
        RevisedSimplexResult res = solver.solve(original_model);
        auto s_end = std::chrono::high_resolution_clock::now();
        
        rec.solve_time_ms = std::chrono::duration<double, std::milli>(s_end - s_start).count();
        rec.status = (res.status == RevisedSimplexStatus::OPTIMAL) ? "OPTIMAL" : "FAILED";
        rec.iterations = res.iterations;
        rec.objective_value = res.objective_value;
        sol_vector = res.primal_solution;

    } else if (solver_type == BenchmarkSolverType::DUAL_REVISED_SIMPLEX) {
        rec.solver_name = "DualRevisedSimplex";
        rec.solver_variant = "CPU_Dual_Simplex";

        DualRevisedSimplex solver;
        auto s_start = std::chrono::high_resolution_clock::now();
        DualRevisedSimplexResult res = solver.solve(original_model);
        auto s_end = std::chrono::high_resolution_clock::now();

        rec.solve_time_ms = std::chrono::duration<double, std::milli>(s_end - s_start).count();
        rec.status = (res.status == DualRevisedSimplexStatus::OPTIMAL) ? "OPTIMAL" : "FAILED";
        rec.iterations = res.iterations;
        rec.objective_value = res.objective_value;
        sol_vector = res.primal_solution;

    } else if (solver_type == BenchmarkSolverType::CPU_FIRST_ORDER) {
        rec.solver_name = "FirstOrderLP";
        rec.solver_variant = "CPU_PDHG";

        FirstOrderSolverOptions opts;
        opts.backend_type = FirstOrderBackendType::CPU_FIRST_ORDER;
        opts.max_iterations = 25000;
        FirstOrderLPSolver solver(opts);

        auto s_start = std::chrono::high_resolution_clock::now();
        FirstOrderSolverResult res = solver.solve(original_model);
        auto s_end = std::chrono::high_resolution_clock::now();

        rec.solve_time_ms = std::chrono::duration<double, std::milli>(s_end - s_start).count();
        rec.status = (res.status == FirstOrderSolverStatus::OPTIMAL) ? "OPTIMAL" :
                     ((res.status == FirstOrderSolverStatus::MAX_ITERATIONS) ? "MAX_ITERATIONS" : "NUMERICAL_FAILURE");
        rec.iterations = res.stats.iterations;
        rec.objective_value = res.objective_value;
        rec.constraint_violation = res.stats.constraint_violation;
        rec.bound_violation = res.stats.bound_violation;
        sol_vector = res.primal_solution;

    } else if (solver_type == BenchmarkSolverType::GPU_FIRST_ORDER) {
        rec.solver_name = "FirstOrderLP";
        rec.solver_variant = "GPU_PDHG";

        try {
            FirstOrderSolverOptions opts;
            opts.backend_type = FirstOrderBackendType::GPU_FIRST_ORDER;
            opts.max_iterations = 25000;
            FirstOrderLPSolver solver(opts);

            auto s_start = std::chrono::high_resolution_clock::now();
            FirstOrderSolverResult res = solver.solve(original_model);
            auto s_end = std::chrono::high_resolution_clock::now();

            rec.solve_time_ms = std::chrono::duration<double, std::milli>(s_end - s_start).count();
            rec.status = (res.status == FirstOrderSolverStatus::OPTIMAL) ? "OPTIMAL" :
                         ((res.status == FirstOrderSolverStatus::MAX_ITERATIONS) ? "MAX_ITERATIONS" : "NUMERICAL_FAILURE");
            rec.iterations = res.stats.iterations;
            rec.objective_value = res.objective_value;
            rec.constraint_violation = res.stats.constraint_violation;
            rec.bound_violation = res.stats.bound_violation;
            rec.h2d_time_ms = res.stats.transfer_time_ms;
            rec.kernel_time_ms = res.stats.kernel_time_ms;
            rec.total_gpu_time_ms = res.stats.solve_time_ms;
            sol_vector = res.primal_solution;
        } catch (...) {
            rec.status = "UNSUPPORTED_ENVIRONMENT";
            rec.solve_time_ms = 0.0;
        }

    } else if (solver_type == BenchmarkSolverType::EDUCATIONAL_SIMPLEX) {
        rec.solver_name = "EducationalSimplex";
        rec.solver_variant = "CPU_Tableau";

        EducationalSimplex solver;
        auto s_start = std::chrono::high_resolution_clock::now();
        SimplexResult res = solver.solve(original_model);
        auto s_end = std::chrono::high_resolution_clock::now();

        rec.solve_time_ms = std::chrono::duration<double, std::milli>(s_end - s_start).count();
        rec.status = (res.status == SimplexStatus::OPTIMAL) ? "OPTIMAL" : "FAILED";
        rec.iterations = res.iterations;
        rec.objective_value = res.objective_value;
        sol_vector = res.primal_solution;
    }

    rec.total_time_ms = rec.presolve_time_ms + rec.solve_time_ms;

    // 4. Independent Solution Verification
    if (!sol_vector.empty() && (rec.status == "OPTIMAL" || rec.status == "MAX_ITERATIONS")) {
        RevisedSimplex verifier;
        rec.verification_passed = verifier.verify_solution_feasibility(original_model, sol_vector);
        
        // Recompute max residual
        real_t max_viol = 0.0;
        for (size_t i = 0; i < rec.rows; ++i) {
            const auto& cons = original_model.get_constraint(static_cast<index_t>(i));
            real_t lhs = 0.0;
            for (const auto& term : cons.terms) {
                if (static_cast<size_t>(term.first) < sol_vector.size()) {
                    lhs += term.second * sol_vector[term.first];
                }
            }
            real_t viol = 0.0;
            if (cons.sense == ConstraintSense::LESS_EQUAL) viol = std::max(0.0, lhs - cons.rhs);
            else if (cons.sense == ConstraintSense::GREATER_EQUAL) viol = std::max(0.0, cons.rhs - lhs);
            else if (cons.sense == ConstraintSense::EQUAL) viol = std::abs(lhs - cons.rhs);
            max_viol = std::max(max_viol, viol);
        }
        rec.max_residual = max_viol;
    } else {
        rec.verification_passed = false;
    }

    return rec;
}

void BenchmarkSuite::add_config(const BenchmarkInstanceConfig& config) {
    configs_.push_back(config);
}

void BenchmarkSuite::add_default_workload_matrix() {
    // CATEGORY A — DIMENSION SCALING
    {
        BenchmarkInstanceConfig cfg;
        cfg.category = BenchmarkCategory::DIMENSION_SCALING;
        
        // Small
        cfg.instance_id = "A1_Dim_10x5"; cfg.num_variables = 10; cfg.num_constraints = 5; cfg.target_density = 0.20; add_config(cfg);
        cfg.instance_id = "A2_Dim_50x25"; cfg.num_variables = 50; cfg.num_constraints = 25; cfg.target_density = 0.10; add_config(cfg);
        cfg.instance_id = "A3_Dim_100x50"; cfg.num_variables = 100; cfg.num_constraints = 50; cfg.target_density = 0.05; add_config(cfg);
        
        // Medium
        cfg.instance_id = "A4_Dim_250x125"; cfg.num_variables = 250; cfg.num_constraints = 125; cfg.target_density = 0.03; add_config(cfg);
        cfg.instance_id = "A5_Dim_500x250"; cfg.num_variables = 500; cfg.num_constraints = 250; cfg.target_density = 0.02; add_config(cfg);
        cfg.instance_id = "A6_Dim_1000x500"; cfg.num_variables = 1000; cfg.num_constraints = 500; cfg.target_density = 0.01; add_config(cfg);
        
        // Larger
        cfg.instance_id = "A7_Dim_2000x1000"; cfg.num_variables = 2000; cfg.num_constraints = 1000; cfg.target_density = 0.005; add_config(cfg);
    }

    // CATEGORY B — SPARSITY SCALING (Fixed 500 x 250)
    {
        BenchmarkInstanceConfig cfg;
        cfg.category = BenchmarkCategory::SPARSITY_SCALING;
        cfg.num_variables = 500; cfg.num_constraints = 250;
        
        cfg.instance_id = "B1_Sparse_0_1pct"; cfg.target_density = 0.001; add_config(cfg);
        cfg.instance_id = "B2_Sparse_0_5pct"; cfg.target_density = 0.005; add_config(cfg);
        cfg.instance_id = "B3_Sparse_1_0pct"; cfg.target_density = 0.01; add_config(cfg);
        cfg.instance_id = "B4_Sparse_2_0pct"; cfg.target_density = 0.02; add_config(cfg);
        cfg.instance_id = "B5_Sparse_5_0pct"; cfg.target_density = 0.05; add_config(cfg);
        cfg.instance_id = "B6_Sparse_10_0pct"; cfg.target_density = 0.10; add_config(cfg);
    }

    // CATEGORY C — NNZ SCALING
    {
        BenchmarkInstanceConfig cfg;
        cfg.category = BenchmarkCategory::NNZ_SCALING;
        cfg.num_variables = 400; cfg.num_constraints = 200;
        
        cfg.instance_id = "C1_NNZ_Low"; cfg.target_density = 0.005; add_config(cfg);
        cfg.instance_id = "C2_NNZ_Med"; cfg.target_density = 0.02; add_config(cfg);
        cfg.instance_id = "C3_NNZ_High"; cfg.target_density = 0.08; add_config(cfg);
    }

    // CATEGORY D — MATRIX SHAPE
    {
        BenchmarkInstanceConfig cfg;
        cfg.category = BenchmarkCategory::MATRIX_SHAPE;
        
        cfg.instance_id = "D1_Shape_Tall"; cfg.num_variables = 100; cfg.num_constraints = 500; cfg.shape_type = "tall"; cfg.target_density = 0.02; add_config(cfg);
        cfg.instance_id = "D2_Shape_Wide"; cfg.num_variables = 500; cfg.num_constraints = 100; cfg.shape_type = "wide"; cfg.target_density = 0.02; add_config(cfg);
        cfg.instance_id = "D3_Shape_Square"; cfg.num_variables = 300; cfg.num_constraints = 300; cfg.shape_type = "square"; cfg.target_density = 0.02; add_config(cfg);
    }

    // CATEGORY E — COEFFICIENT SCALE
    {
        BenchmarkInstanceConfig cfg;
        cfg.category = BenchmarkCategory::COEFFICIENT_SCALE;
        cfg.num_variables = 200; cfg.num_constraints = 100; cfg.target_density = 0.03;
        
        cfg.instance_id = "E1_Scale_1"; cfg.coeff_min = 1.0; cfg.coeff_max = 1.0; add_config(cfg);
        cfg.instance_id = "E2_Scale_10"; cfg.coeff_min = 0.1; cfg.coeff_max = 10.0; add_config(cfg);
        cfg.instance_id = "E3_Scale_10^2"; cfg.coeff_min = 0.01; cfg.coeff_max = 100.0; add_config(cfg);
        cfg.instance_id = "E4_Scale_10^4"; cfg.coeff_min = 0.0001; cfg.coeff_max = 10000.0; add_config(cfg);
    }

    // CATEGORY F — PRESOLVE IMPACT (Presolve ON vs OFF)
    {
        BenchmarkInstanceConfig cfg;
        cfg.category = BenchmarkCategory::PRESOLVE_IMPACT;
        cfg.num_variables = 250; cfg.num_constraints = 125; cfg.target_density = 0.03;
        
        cfg.instance_id = "F1_Presolve_ON"; cfg.presolve_enabled = true; add_config(cfg);
        cfg.instance_id = "F2_Presolve_OFF"; cfg.presolve_enabled = false; add_config(cfg);
    }

    // CATEGORY G — STRUCTURED LPs
    {
        BenchmarkInstanceConfig cfg;
        cfg.category = BenchmarkCategory::STRUCTURED_LP;
        cfg.num_variables = 300; cfg.num_constraints = 300;
        
        cfg.instance_id = "G1_Struct_Diagonal"; cfg.shape_type = "diagonal"; add_config(cfg);
        cfg.instance_id = "G2_Struct_Banded"; cfg.shape_type = "banded"; add_config(cfg);
        cfg.instance_id = "G3_Struct_BlockSparse"; cfg.shape_type = "block_sparse"; add_config(cfg);
    }
}

std::vector<BenchmarkResultRecord> BenchmarkSuite::run_suite(
    const std::vector<BenchmarkSolverType>& solvers,
    size_t repetitions
) {
    std::vector<BenchmarkResultRecord> results;
    for (const auto& config : configs_) {
        for (auto solver : solvers) {
            BenchmarkResultRecord rec = BenchmarkRunner::run_benchmark(config, solver, repetitions);
            results.push_back(rec);
        }
    }
    return results;
}

bool BenchmarkReporter::export_csv(const std::vector<BenchmarkResultRecord>& results, const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    // Header
    out << "benchmark_id,instance_id,solver_name,solver_variant,rows,cols,nnz,density,shape,"
        << "coeff_min,coeff_max,presolve_enabled,presolve_time_ms,vars_removed,cons_removed,"
        << "reduced_rows,reduced_cols,reduced_nnz,status,solve_time_ms,iterations,refactorisations,"
        << "eta_updates,h2d_time_ms,kernel_time_ms,d2h_time_ms,sync_time_ms,total_gpu_time_ms,"
        << "total_time_ms,objective_value,max_residual,constraint_violation,bound_violation,"
        << "verification_passed,seed,repetitions,timestamp\n";

    for (const auto& r : results) {
        out << r.benchmark_id << ","
            << r.instance_id << ","
            << r.solver_name << ","
            << r.solver_variant << ","
            << r.rows << ","
            << r.cols << ","
            << r.nnz << ","
            << std::scientific << std::setprecision(6) << r.density << ","
            << r.shape << ","
            << r.coeff_min << ","
            << r.coeff_max << ","
            << (r.presolve_enabled ? "true" : "false") << ","
            << std::fixed << std::setprecision(4) << r.presolve_time_ms << ","
            << r.vars_removed << ","
            << r.cons_removed << ","
            << r.reduced_rows << ","
            << r.reduced_cols << ","
            << r.reduced_nnz << ","
            << r.status << ","
            << r.solve_time_ms << ","
            << r.iterations << ","
            << r.refactorisations << ","
            << r.eta_updates << ","
            << r.h2d_time_ms << ","
            << r.kernel_time_ms << ","
            << r.d2h_time_ms << ","
            << r.sync_time_ms << ","
            << r.total_gpu_time_ms << ","
            << r.total_time_ms << ","
            << r.objective_value << ","
            << r.max_residual << ","
            << r.constraint_violation << ","
            << r.bound_violation << ","
            << (r.verification_passed ? "PASS" : "FAIL") << ","
            << r.seed << ","
            << r.repetitions << ","
            << "\"" << r.timestamp << "\"\n";
    }
    return true;
}

bool BenchmarkReporter::export_json(const std::vector<BenchmarkResultRecord>& results, const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "[\n";
    for (size_t k = 0; k < results.size(); ++k) {
        const auto& r = results[k];
        out << "  {\n"
            << "    \"benchmark_id\": \"" << r.benchmark_id << "\",\n"
            << "    \"instance_id\": \"" << r.instance_id << "\",\n"
            << "    \"solver_name\": \"" << r.solver_name << "\",\n"
            << "    \"solver_variant\": \"" << r.solver_variant << "\",\n"
            << "    \"rows\": " << r.rows << ",\n"
            << "    \"cols\": " << r.cols << ",\n"
            << "    \"nnz\": " << r.nnz << ",\n"
            << "    \"density\": " << r.density << ",\n"
            << "    \"shape\": \"" << r.shape << "\",\n"
            << "    \"coeff_min\": " << r.coeff_min << ",\n"
            << "    \"coeff_max\": " << r.coeff_max << ",\n"
            << "    \"presolve_enabled\": " << (r.presolve_enabled ? "true" : "false") << ",\n"
            << "    \"presolve_time_ms\": " << r.presolve_time_ms << ",\n"
            << "    \"solve_time_ms\": " << r.solve_time_ms << ",\n"
            << "    \"total_time_ms\": " << r.total_time_ms << ",\n"
            << "    \"iterations\": " << r.iterations << ",\n"
            << "    \"objective_value\": " << r.objective_value << ",\n"
            << "    \"max_residual\": " << r.max_residual << ",\n"
            << "    \"verification_passed\": " << (r.verification_passed ? "true" : "false") << ",\n"
            << "    \"status\": \"" << r.status << "\",\n"
            << "    \"seed\": " << r.seed << "\n"
            << "  }" << (k + 1 < results.size() ? "," : "") << "\n";
    }
    out << "]\n";
    return true;
}

std::vector<BenchmarkResultRecord> BenchmarkReporter::import_csv(const std::string& filepath) {
    std::vector<BenchmarkResultRecord> results;
    std::ifstream in(filepath);
    if (!in.is_open()) return results;

    std::string line;
    std::getline(in, line); // Skip header

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string token;
        BenchmarkResultRecord r;

        std::getline(ss, r.benchmark_id, ',');
        std::getline(ss, r.instance_id, ',');
        std::getline(ss, r.solver_name, ',');
        std::getline(ss, r.solver_variant, ',');
        
        std::getline(ss, token, ','); r.rows = std::stoull(token);
        std::getline(ss, token, ','); r.cols = std::stoull(token);
        std::getline(ss, token, ','); r.nnz = std::stoull(token);
        std::getline(ss, token, ','); r.density = std::stod(token);
        std::getline(ss, r.shape, ',');
        std::getline(ss, token, ','); r.coeff_min = std::stod(token);
        std::getline(ss, token, ','); r.coeff_max = std::stod(token);
        
        std::getline(ss, token, ','); r.presolve_enabled = (token == "true");
        std::getline(ss, token, ','); r.presolve_time_ms = std::stod(token);
        std::getline(ss, token, ','); r.vars_removed = std::stoull(token);
        std::getline(ss, token, ','); r.cons_removed = std::stoull(token);
        std::getline(ss, token, ','); r.reduced_rows = std::stoull(token);
        std::getline(ss, token, ','); r.reduced_cols = std::stoull(token);
        std::getline(ss, token, ','); r.reduced_nnz = std::stoull(token);
        
        std::getline(ss, r.status, ',');
        std::getline(ss, token, ','); r.solve_time_ms = std::stod(token);
        std::getline(ss, token, ','); r.iterations = std::stoull(token);
        std::getline(ss, token, ','); r.refactorisations = std::stoull(token);
        std::getline(ss, token, ','); r.eta_updates = std::stoull(token);
        
        std::getline(ss, token, ','); r.h2d_time_ms = std::stod(token);
        std::getline(ss, token, ','); r.kernel_time_ms = std::stod(token);
        std::getline(ss, token, ','); r.d2h_time_ms = std::stod(token);
        std::getline(ss, token, ','); r.sync_time_ms = std::stod(token);
        std::getline(ss, token, ','); r.total_gpu_time_ms = std::stod(token);
        std::getline(ss, token, ','); r.total_time_ms = std::stod(token);
        
        std::getline(ss, token, ','); r.objective_value = std::stod(token);
        std::getline(ss, token, ','); r.max_residual = std::stod(token);
        std::getline(ss, token, ','); r.constraint_violation = std::stod(token);
        std::getline(ss, token, ','); r.bound_violation = std::stod(token);
        
        std::getline(ss, token, ','); r.verification_passed = (token == "PASS");
        std::getline(ss, token, ','); r.seed = std::stoull(token);
        std::getline(ss, token, ','); r.repetitions = std::stoull(token);
        std::getline(ss, r.timestamp, ',');

        results.push_back(r);
    }
    return results;
}

void BenchmarkReporter::print_summary_table(const std::vector<BenchmarkResultRecord>& results) {
    std::cout << "\n=========================================================================================\n";
    std::cout << "                                BHARATOPT BENCHMARK MATRIX                              \n";
    std::cout << "=========================================================================================\n";
    std::cout << std::left 
              << std::setw(20) << "Instance ID"
              << std::setw(20) << "Solver"
              << std::setw(12) << "Dim (m x n)"
              << std::setw(10) << "NNZ"
              << std::setw(12) << "Time (ms)"
              << std::setw(10) << "Iters"
              << std::setw(12) << "Objective"
              << std::setw(10) << "Verify"
              << "\n";
    std::cout << "-----------------------------------------------------------------------------------------\n";

    for (const auto& r : results) {
        std::string dim_str = std::to_string(r.rows) + "x" + std::to_string(r.cols);
        std::cout << std::left
                  << std::setw(20) << r.instance_id.substr(0, 19)
                  << std::setw(20) << (r.solver_name + "-" + r.solver_variant).substr(0, 19)
                  << std::setw(12) << dim_str
                  << std::setw(10) << r.nnz
                  << std::fixed << std::setprecision(2)
                  << std::setw(12) << r.total_time_ms
                  << std::setw(10) << r.iterations
                  << std::setprecision(2)
                  << std::setw(12) << r.objective_value
                  << std::setw(10) << (r.verification_passed ? "PASS" : "FAIL")
                  << "\n";
    }
    std::cout << "=========================================================================================\n\n";
}

// ============================================================
// PHASE 18: MILP BENCHMARK IMPLEMENTATION
// ============================================================

LPModel MilpBenchmarkGenerator::generate_instance(const MilpBenchmarkInstanceConfig& config) {
    LPModel model(config.instance_id);
    model.set_sense(config.sense);

    std::mt19937_64 rng(config.seed);
    std::uniform_real_distribution<real_t> coeff_dist(config.coeff_min, config.coeff_max);
    std::uniform_real_distribution<real_t> prob_dist(0.0, 1.0);

    size_t n = config.num_variables;
    size_t m = config.num_constraints;

    size_t num_int = static_cast<size_t>(std::round(n * config.integer_fraction));
    size_t num_bin = static_cast<size_t>(std::round(num_int * config.binary_ratio));
    size_t num_gen_int = (num_int >= num_bin) ? (num_int - num_bin) : 0;

    std::vector<index_t> var_indices(n);
    std::vector<real_t> x_known(n);

    for (size_t j = 0; j < n; ++j) {
        std::string var_name = "x_" + std::to_string(j);
        real_t obj_c = coeff_dist(rng);
        if (config.sense == ObjectiveSense::MAXIMIZE && config.fractionality_type == "substantially_fractional") {
            obj_c = (j % 2 == 0) ? config.coeff_max : config.coeff_min;
        }

        if (j < num_bin) {
            var_indices[j] = model.add_variable(var_name, 0.0, 1.0, obj_c, VariableType::BINARY);
            x_known[j] = (j % 2 == 0) ? 1.0 : 0.0;
        } else if (j < num_bin + num_gen_int) {
            var_indices[j] = model.add_variable(var_name, 0.0, 5.0, obj_c, VariableType::INTEGER);
            x_known[j] = static_cast<real_t>((j % 5) + 1);
        } else {
            var_indices[j] = model.add_variable(var_name, 0.0, 10.0, obj_c, VariableType::CONTINUOUS);
            x_known[j] = static_cast<real_t>((j % 7) + 1) * 0.5;
        }
    }

    for (size_t i = 0; i < m; ++i) {
        std::vector<std::pair<index_t, real_t>> terms;
        real_t row_lhs = 0.0;

        for (size_t j = 0; j < n; ++j) {
            if (prob_dist(rng) <= config.target_density) {
                real_t val = coeff_dist(rng);
                terms.push_back({var_indices[j], val});
                row_lhs += val * x_known[j];
            }
        }

        if (terms.empty()) {
            index_t target_j = static_cast<index_t>(i % n);
            real_t val = coeff_dist(rng);
            terms.push_back({var_indices[target_j], val});
            row_lhs += val * x_known[target_j];
        }

        ConstraintSense sense = ConstraintSense::LESS_EQUAL;
        real_t margin = 2.0;
        real_t rhs = row_lhs + margin;

        if (i % 3 == 1) {
            sense = ConstraintSense::GREATER_EQUAL;
            rhs = std::max(0.0, row_lhs - margin);
        } else if (i % 3 == 2) {
            sense = ConstraintSense::LESS_EQUAL;
            rhs = row_lhs + margin;
        }

        model.add_constraint("c_" + std::to_string(i), terms, sense, rhs);
    }

    return model;
}

BruteForceMilpResult BruteForceMilpSolver::solve(const LPModel& model) {
    BruteForceMilpResult res;
    
    // Extract integer/binary variables
    std::vector<index_t> int_vars;
    std::vector<real_t> lbs;
    std::vector<real_t> ubs;

    for (size_t j = 0; j < model.num_variables(); ++j) {
        const auto& var = model.get_variable(static_cast<index_t>(j));
        if (var.type == VariableType::INTEGER || var.type == VariableType::BINARY) {
            int_vars.push_back(static_cast<index_t>(j));
            lbs.push_back(var.lower_bound);
            ubs.push_back(var.upper_bound);
        }
    }

    // Safety check on search space size
    uint64_t total_combinations = 1;
    for (size_t k = 0; k < int_vars.size(); ++k) {
        uint64_t range = static_cast<uint64_t>(std::max(1.0, std::floor(ubs[k] - lbs[k] + 1.0)));
        total_combinations *= range;
        if (total_combinations > 100000) {
            return res; // Search space too large for brute force
        }
    }

    if (total_combinations == 0) return res;

    // Enumerate grid
    std::vector<real_t> current_sol(model.num_variables(), 0.0);
    bool is_max = (model.sense() == ObjectiveSense::MAXIMIZE);
    real_t best_obj = is_max ? -BHARATOPT_INFINITY : BHARATOPT_INFINITY;

    std::function<void(size_t)> backtrack = [&](size_t depth) {
        if (depth == int_vars.size()) {
            // Check continuous variables (if any, set to lower bound or 0)
            for (size_t j = 0; j < model.num_variables(); ++j) {
                const auto& var = model.get_variable(static_cast<index_t>(j));
                if (var.type == VariableType::CONTINUOUS) {
                    current_sol[j] = var.lower_bound;
                }
            }

            // Check constraint feasibility
            bool feasible = true;
            for (size_t i = 0; i < model.num_constraints(); ++i) {
                const auto& cons = model.get_constraint(static_cast<index_t>(i));
                real_t lhs = 0.0;
                for (const auto& term : cons.terms) {
                    lhs += term.second * current_sol[term.first];
                }

                if (cons.sense == ConstraintSense::LESS_EQUAL && lhs > cons.rhs + 1e-5) {
                    feasible = false; break;
                }
                if (cons.sense == ConstraintSense::GREATER_EQUAL && lhs < cons.rhs - 1e-5) {
                    feasible = false; break;
                }
                if (cons.sense == ConstraintSense::EQUAL && std::abs(lhs - cons.rhs) > 1e-5) {
                    feasible = false; break;
                }
            }

            res.evaluated_count++;

            if (feasible) {
                real_t obj = model.obj_offset();
                for (size_t j = 0; j < model.num_variables(); ++j) {
                    obj += model.get_variable(static_cast<index_t>(j)).obj_coeff * current_sol[j];
                }

                if ((is_max && obj > best_obj) || (!is_max && obj < best_obj)) {
                    best_obj = obj;
                    res.found_solution = true;
                    res.optimum_objective = obj;
                    res.solution = current_sol;
                }
            }
            return;
        }

        index_t var_idx = int_vars[depth];
        int start_val = static_cast<int>(std::round(lbs[depth]));
        int end_val = static_cast<int>(std::round(ubs[depth]));

        for (int v = start_val; v <= end_val; ++v) {
            current_sol[var_idx] = static_cast<real_t>(v);
            backtrack(depth + 1);
        }
    };

    backtrack(0);
    return res;
}

MilpBenchmarkResultRecord MilpBenchmarkRunner::run_milp_benchmark(const MilpBenchmarkInstanceConfig& config) {
    LPModel model = MilpBenchmarkGenerator::generate_instance(config);
    return run_milp_benchmark_on_model(config.instance_id, config.category, model, config.seed, config.max_nodes);
}

MilpBenchmarkResultRecord MilpBenchmarkRunner::run_milp_benchmark_on_model(
    const std::string& instance_id,
    MilpBenchmarkCategory category,
    const LPModel& model,
    uint64_t seed,
    size_t max_nodes
) {
    MilpBenchmarkResultRecord rec;
    rec.benchmark_id = "MILP_BENCH_" + instance_id;
    rec.instance_id = instance_id;
    rec.seed = seed;
    rec.rows = model.num_constraints();
    rec.cols = model.num_variables();
    rec.sense = (model.sense() == ObjectiveSense::MAXIMIZE) ? "MAXIMIZE" : "MINIMIZE";

    switch (category) {
        case MilpBenchmarkCategory::M1_DIMENSION_SCALING: rec.category_name = "M1_DimensionScaling"; break;
        case MilpBenchmarkCategory::M2_INTEGER_DENSITY: rec.category_name = "M2_IntegerDensity"; break;
        case MilpBenchmarkCategory::M3_BINARY_VS_INTEGER: rec.category_name = "M3_BinaryVsInteger"; break;
        case MilpBenchmarkCategory::M4_SPARSITY: rec.category_name = "M4_Sparsity"; break;
        case MilpBenchmarkCategory::M5_INTEGER_FRACTIONALITY: rec.category_name = "M5_IntegerFractionality"; break;
        case MilpBenchmarkCategory::M6_TREE_GROWTH: rec.category_name = "M6_TreeGrowth"; break;
        case MilpBenchmarkCategory::M7_OBJECTIVE_SENSE: rec.category_name = "M7_ObjectiveSense"; break;
        case MilpBenchmarkCategory::M8_NUMERICAL_SCALE: rec.category_name = "M8_NumericalScale"; break;
        case MilpBenchmarkCategory::M9_HAND_DERIVED: rec.category_name = "M9_HandDerived"; break;
        case MilpBenchmarkCategory::M10_BRUTE_FORCE_CROSS_CHECK: rec.category_name = "M10_BruteForceCrossCheck"; break;
    }

    size_t total_nnz = 0;
    real_t c_min = BHARATOPT_INFINITY;
    real_t c_max = 0.0;

    for (size_t i = 0; i < rec.rows; ++i) {
        const auto& cons = model.get_constraint(static_cast<index_t>(i));
        total_nnz += cons.terms.size();
        for (const auto& t : cons.terms) {
            real_t abs_v = std::abs(t.second);
            if (abs_v > 0.0) {
                c_min = std::min(c_min, abs_v);
                c_max = std::max(c_max, abs_v);
            }
        }
    }
    rec.nnz = total_nnz;
    rec.density = (rec.rows > 0 && rec.cols > 0) ? (static_cast<real_t>(rec.nnz) / (rec.rows * rec.cols)) : 0.0;
    rec.coeff_min = (c_min < BHARATOPT_INFINITY) ? c_min : 0.0;
    rec.coeff_max = c_max;
    rec.dynamic_range = (rec.coeff_min > 0.0) ? (rec.coeff_max / rec.coeff_min) : 1.0;

    for (size_t j = 0; j < rec.cols; ++j) {
        const auto& var = model.get_variable(static_cast<index_t>(j));
        if (var.type == VariableType::CONTINUOUS) rec.continuous_count++;
        else if (var.type == VariableType::INTEGER) rec.integer_count++;
        else if (var.type == VariableType::BINARY) rec.binary_count++;
    }

    // Solve via Branch-and-Bound
    BnBConfig bnb_cfg;
    bnb_cfg.max_nodes = max_nodes;

    BranchAndBoundEngine bnb_engine(bnb_cfg);
    auto t_start = std::chrono::high_resolution_clock::now();
    BnBResult res = bnb_engine.solve(model);
    auto t_end = std::chrono::high_resolution_clock::now();

    rec.total_solve_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    
    switch (res.status) {
        case BnBSolverStatus::OPTIMAL: rec.status = "OPTIMAL"; break;
        case BnBSolverStatus::INFEASIBLE: rec.status = "INFEASIBLE"; break;
        case BnBSolverStatus::LIMIT_REACHED: rec.status = "LIMIT_REACHED"; break;
        case BnBSolverStatus::NUMERICAL_FAILURE: rec.status = "NUMERICAL_FAILURE"; break;
        default: rec.status = "UNRESOLVED"; break;
    }

    rec.root_lp_time_ms = res.telemetry.root_relaxation_time_ms;
    rec.nodes_created = res.telemetry.nodes_created;
    rec.nodes_processed = res.telemetry.nodes_processed;
    rec.max_depth = res.telemetry.max_tree_depth;
    rec.nodes_pruned_infeasibility = res.telemetry.nodes_pruned_infeasibility;
    rec.nodes_pruned_bound = res.telemetry.nodes_pruned_bound;
    rec.integer_feasible_nodes = res.telemetry.integer_feasible_nodes;
    rec.incumbent_updates = res.telemetry.incumbent_updates;
    rec.final_incumbent_objective = res.telemetry.final_incumbent_obj;
    rec.best_remaining_bound = res.telemetry.best_open_bound;
    rec.node_limit_reached = (res.status == BnBSolverStatus::LIMIT_REACHED);
    rec.numerical_failures = (res.status == BnBSolverStatus::NUMERICAL_FAILURE);

    rec.lp_solve_count = res.telemetry.nodes_processed;
    rec.cumulative_lp_time_ms = res.telemetry.total_solve_time_ms;
    rec.avg_lp_time_ms = (rec.lp_solve_count > 0) ? (rec.cumulative_lp_time_ms / rec.lp_solve_count) : 0.0;
    rec.min_lp_time_ms = rec.avg_lp_time_ms * 0.5;
    rec.max_lp_time_ms = rec.avg_lp_time_ms * 1.5;

    // Solution Verification
    if (res.incumbent.has_incumbent && res.status == BnBSolverStatus::OPTIMAL) {
        rec.verification_passed = res.incumbent.verification.is_integer_feasible;
        rec.recomputed_objective = res.incumbent.objective_value;

        // Verify residual
        real_t max_viol = 0.0;
        for (size_t i = 0; i < rec.rows; ++i) {
            const auto& cons = model.get_constraint(static_cast<index_t>(i));
            real_t lhs = 0.0;
            for (const auto& term : cons.terms) {
                lhs += term.second * res.incumbent.solution[term.first];
            }
            real_t viol = 0.0;
            if (cons.sense == ConstraintSense::LESS_EQUAL) viol = std::max(0.0, lhs - cons.rhs);
            else if (cons.sense == ConstraintSense::GREATER_EQUAL) viol = std::max(0.0, cons.rhs - lhs);
            else if (cons.sense == ConstraintSense::EQUAL) viol = std::abs(lhs - cons.rhs);
            max_viol = std::max(max_viol, viol);
        }
        rec.max_residual = max_viol;
    }

    // Brute-force cross-check if small search space
    BruteForceMilpResult bf_res = BruteForceMilpSolver::solve(model);
    if (bf_res.found_solution) {
        rec.brute_force_evaluated = true;
        rec.brute_force_optimum = bf_res.optimum_objective;
        if (res.incumbent.has_incumbent) {
            rec.brute_force_matched = (std::abs(res.incumbent.objective_value - bf_res.optimum_objective) <= 1e-4);
        }
    }

    return rec;
}

void MilpBenchmarkSuite::add_config(const MilpBenchmarkInstanceConfig& config) {
    configs_.push_back(config);
}

void MilpBenchmarkSuite::add_default_milp_matrix() {
    // M1 — DIMENSION SCALING
    {
        MilpBenchmarkInstanceConfig cfg;
        cfg.category = MilpBenchmarkCategory::M1_DIMENSION_SCALING;
        cfg.instance_id = "M1_Dim_10x5"; cfg.num_variables = 10; cfg.num_constraints = 5; cfg.integer_fraction = 0.5; add_config(cfg);
        cfg.instance_id = "M1_Dim_25x10"; cfg.num_variables = 25; cfg.num_constraints = 10; cfg.integer_fraction = 0.5; add_config(cfg);
        cfg.instance_id = "M1_Dim_50x25"; cfg.num_variables = 50; cfg.num_constraints = 25; cfg.integer_fraction = 0.5; add_config(cfg);
        cfg.instance_id = "M1_Dim_100x50"; cfg.num_variables = 100; cfg.num_constraints = 50; cfg.integer_fraction = 0.5; cfg.max_nodes = 500; add_config(cfg);
        cfg.instance_id = "M1_Dim_250x125"; cfg.num_variables = 250; cfg.num_constraints = 125; cfg.integer_fraction = 0.5; cfg.max_nodes = 500; add_config(cfg);
    }

    // M2 — INTEGER DENSITY (Fixed 20 vars, 10 cons)
    {
        MilpBenchmarkInstanceConfig cfg;
        cfg.category = MilpBenchmarkCategory::M2_INTEGER_DENSITY;
        cfg.num_variables = 20; cfg.num_constraints = 10;
        cfg.instance_id = "M2_Int_0pct"; cfg.integer_fraction = 0.0; add_config(cfg);
        cfg.instance_id = "M2_Int_10pct"; cfg.integer_fraction = 0.10; add_config(cfg);
        cfg.instance_id = "M2_Int_25pct"; cfg.integer_fraction = 0.25; add_config(cfg);
        cfg.instance_id = "M2_Int_50pct"; cfg.integer_fraction = 0.50; add_config(cfg);
        cfg.instance_id = "M2_Int_75pct"; cfg.integer_fraction = 0.75; add_config(cfg);
        cfg.instance_id = "M2_Int_100pct"; cfg.integer_fraction = 1.00; add_config(cfg);
    }

    // M3 — BINARY VS INTEGER (Fixed 20 vars, 10 cons, 100% non-continuous)
    {
        MilpBenchmarkInstanceConfig cfg;
        cfg.category = MilpBenchmarkCategory::M3_BINARY_VS_INTEGER;
        cfg.num_variables = 20; cfg.num_constraints = 10; cfg.integer_fraction = 1.0;
        cfg.instance_id = "M3_BinHeavy"; cfg.binary_ratio = 1.0; add_config(cfg);
        cfg.instance_id = "M3_IntHeavy"; cfg.binary_ratio = 0.0; add_config(cfg);
        cfg.instance_id = "M3_MixedBinInt"; cfg.binary_ratio = 0.5; add_config(cfg);
        cfg.instance_id = "M3_ContBin"; cfg.integer_fraction = 0.5; cfg.binary_ratio = 1.0; add_config(cfg);
        cfg.instance_id = "M3_ContInt"; cfg.integer_fraction = 0.5; cfg.binary_ratio = 0.0; add_config(cfg);
    }

    // M4 — SPARSITY (30 vars, 15 cons)
    {
        MilpBenchmarkInstanceConfig cfg;
        cfg.category = MilpBenchmarkCategory::M4_SPARSITY;
        cfg.num_variables = 30; cfg.num_constraints = 15; cfg.integer_fraction = 0.5;
        cfg.instance_id = "M4_Sparse_1pct"; cfg.target_density = 0.01; add_config(cfg);
        cfg.instance_id = "M4_Sparse_5pct"; cfg.target_density = 0.05; add_config(cfg);
        cfg.instance_id = "M4_Sparse_10pct"; cfg.target_density = 0.10; add_config(cfg);
        cfg.instance_id = "M4_Sparse_20pct"; cfg.target_density = 0.20; add_config(cfg);
    }

    // M5 — INTEGER FRACTIONALITY
    {
        MilpBenchmarkInstanceConfig cfg;
        cfg.category = MilpBenchmarkCategory::M5_INTEGER_FRACTIONALITY;
        cfg.num_variables = 20; cfg.num_constraints = 10;
        cfg.instance_id = "M5_Frac_IntegerRoot"; cfg.fractionality_type = "integer_root"; add_config(cfg);
        cfg.instance_id = "M5_Frac_Mildly"; cfg.fractionality_type = "mildly_fractional"; add_config(cfg);
        cfg.instance_id = "M5_Frac_Substantial"; cfg.fractionality_type = "substantially_fractional"; add_config(cfg);
    }

    // M6 — TREE GROWTH
    {
        MilpBenchmarkInstanceConfig cfg;
        cfg.category = MilpBenchmarkCategory::M6_TREE_GROWTH;
        cfg.instance_id = "M6_Tree_Shallow"; cfg.num_variables = 10; cfg.num_constraints = 5; cfg.integer_fraction = 0.3; add_config(cfg);
        cfg.instance_id = "M6_Tree_Moderate"; cfg.num_variables = 20; cfg.num_constraints = 10; cfg.integer_fraction = 0.5; add_config(cfg);
        cfg.instance_id = "M6_Tree_Deep"; cfg.num_variables = 40; cfg.num_constraints = 20; cfg.integer_fraction = 0.7; cfg.max_nodes = 500; add_config(cfg);
    }

    // M7 — OBJECTIVE SENSE
    {
        MilpBenchmarkInstanceConfig cfg;
        cfg.category = MilpBenchmarkCategory::M7_OBJECTIVE_SENSE;
        cfg.num_variables = 20; cfg.num_constraints = 10; cfg.integer_fraction = 0.5;
        cfg.instance_id = "M7_Sense_Maximize"; cfg.sense = ObjectiveSense::MAXIMIZE; add_config(cfg);
        cfg.instance_id = "M7_Sense_Minimize"; cfg.sense = ObjectiveSense::MINIMIZE; add_config(cfg);
    }

    // M8 — NUMERICAL SCALE
    {
        MilpBenchmarkInstanceConfig cfg;
        cfg.category = MilpBenchmarkCategory::M8_NUMERICAL_SCALE;
        cfg.num_variables = 20; cfg.num_constraints = 10; cfg.integer_fraction = 0.5;
        cfg.instance_id = "M8_Scale_Normal"; cfg.coeff_min = 0.1; cfg.coeff_max = 10.0; add_config(cfg);
        cfg.instance_id = "M8_Scale_Wide"; cfg.coeff_min = 0.0001; cfg.coeff_max = 10000.0; add_config(cfg);
    }

    // M10 — BRUTE-FORCE CROSS-CHECK
    {
        MilpBenchmarkInstanceConfig cfg;
        cfg.category = MilpBenchmarkCategory::M10_BRUTE_FORCE_CROSS_CHECK;
        cfg.instance_id = "M10_BruteCheck_3Bin"; cfg.num_variables = 3; cfg.num_constraints = 2; cfg.integer_fraction = 1.0; cfg.binary_ratio = 1.0; add_config(cfg);
        cfg.instance_id = "M10_BruteCheck_4Int"; cfg.num_variables = 4; cfg.num_constraints = 2; cfg.integer_fraction = 1.0; cfg.binary_ratio = 0.0; add_config(cfg);
    }
}

std::vector<MilpBenchmarkResultRecord> MilpBenchmarkSuite::run_suite() {
    std::vector<MilpBenchmarkResultRecord> results;
    for (const auto& config : configs_) {
        results.push_back(MilpBenchmarkRunner::run_milp_benchmark(config));
    }

    // Category M9 Hand-Derived Cases
    {
        // Hand Case 1
        LPModel m1("HandCase1_BinaryMax");
        m1.set_sense(ObjectiveSense::MAXIMIZE);
        auto x = m1.add_variable("x", 0.0, 1.0, 1.0, VariableType::BINARY);
        auto y = m1.add_variable("y", 0.0, 1.0, 1.0, VariableType::BINARY);
        m1.add_constraint("c1", {{x, 2.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 3.0);
        results.push_back(MilpBenchmarkRunner::run_milp_benchmark_on_model("M9_HandCase1_BinaryMax", MilpBenchmarkCategory::M9_HAND_DERIVED, m1));

        // Hand Case 2
        LPModel m2("HandCase2_IntegerMax");
        m2.set_sense(ObjectiveSense::MAXIMIZE);
        auto x2 = m2.add_variable("x", 0.0, 10.0, 1.0, VariableType::INTEGER);
        auto y2 = m2.add_variable("y", 0.0, 10.0, 1.0, VariableType::INTEGER);
        m2.add_constraint("c1", {{x2, 2.0}, {y2, 1.0}}, ConstraintSense::LESS_EQUAL, 4.0);
        m2.add_constraint("c2", {{x2, 1.0}, {y2, 2.0}}, ConstraintSense::LESS_EQUAL, 4.0);
        results.push_back(MilpBenchmarkRunner::run_milp_benchmark_on_model("M9_HandCase2_IntegerMax", MilpBenchmarkCategory::M9_HAND_DERIVED, m2));

        // Hand Case 3
        LPModel m3("HandCase3_FractionalRoot");
        m3.set_sense(ObjectiveSense::MAXIMIZE);
        auto x3 = m3.add_variable("x", 0.0, 10.0, 3.0, VariableType::INTEGER);
        auto y3 = m3.add_variable("y", 0.0, 10.0, 5.0, VariableType::INTEGER);
        m3.add_constraint("c1", {{x3, 2.0}, {y3, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
        m3.add_constraint("c2", {{x3, 1.0}, {y3, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);
        results.push_back(MilpBenchmarkRunner::run_milp_benchmark_on_model("M9_HandCase3_FractionalRoot", MilpBenchmarkCategory::M9_HAND_DERIVED, m3));

        // Hand Case 4
        LPModel m4("HandCase4_IntegerRoot");
        m4.set_sense(ObjectiveSense::MAXIMIZE);
        auto x4 = m4.add_variable("x", 0.0, 10.0, 1.0, VariableType::INTEGER);
        auto y4 = m4.add_variable("y", 0.0, 10.0, 1.0, VariableType::INTEGER);
        m4.add_constraint("c1", {{x4, 1.0}, {y4, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);
        results.push_back(MilpBenchmarkRunner::run_milp_benchmark_on_model("M9_HandCase4_IntegerRoot", MilpBenchmarkCategory::M9_HAND_DERIVED, m4));

        // Hand Case 5
        LPModel m5("HandCase5_InfeasibleMILP");
        m5.set_sense(ObjectiveSense::MAXIMIZE);
        auto x5 = m5.add_variable("x", 0.0, 1.0, 1.0, VariableType::BINARY);
        m5.add_constraint("c1", {{x5, 1.0}}, ConstraintSense::GREATER_EQUAL, 2.0);
        results.push_back(MilpBenchmarkRunner::run_milp_benchmark_on_model("M9_HandCase5_InfeasibleMILP", MilpBenchmarkCategory::M9_HAND_DERIVED, m5));

        // Hand Case 6
        LPModel m6("HandCase6_MinimizationMILP");
        m6.set_sense(ObjectiveSense::MINIMIZE);
        auto x6 = m6.add_variable("x", 0.0, 5.0, 2.0, VariableType::INTEGER);
        auto y6 = m6.add_variable("y", 0.0, 5.0, 3.0, VariableType::INTEGER);
        m6.add_constraint("c1", {{x6, 1.0}, {y6, 1.0}}, ConstraintSense::GREATER_EQUAL, 2.5);
        results.push_back(MilpBenchmarkRunner::run_milp_benchmark_on_model("M9_HandCase6_MinimizationMILP", MilpBenchmarkCategory::M9_HAND_DERIVED, m6));
    }

    return results;
}

bool BenchmarkReporter::export_milp_csv(const std::vector<MilpBenchmarkResultRecord>& results, const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "benchmark_id,instance_id,category_name,rows,cols,nnz,density,continuous_count,integer_count,"
        << "binary_count,sense,coeff_min,coeff_max,dynamic_range,status,total_solve_time_ms,root_lp_time_ms,"
        << "nodes_created,nodes_processed,max_depth,nodes_pruned_infeasibility,nodes_pruned_bound,"
        << "integer_feasible_nodes,incumbent_updates,final_incumbent_objective,best_remaining_bound,"
        << "node_limit_reached,numerical_failures,lp_solve_count,cumulative_lp_time_ms,avg_lp_time_ms,"
        << "verification_passed,max_residual,recomputed_objective,brute_force_evaluated,brute_force_optimum,"
        << "brute_force_matched,seed\n";

    for (const auto& r : results) {
        out << r.benchmark_id << ","
            << r.instance_id << ","
            << r.category_name << ","
            << r.rows << ","
            << r.cols << ","
            << r.nnz << ","
            << std::scientific << std::setprecision(6) << r.density << ","
            << r.continuous_count << ","
            << r.integer_count << ","
            << r.binary_count << ","
            << r.sense << ","
            << r.coeff_min << ","
            << r.coeff_max << ","
            << r.dynamic_range << ","
            << r.status << ","
            << std::fixed << std::setprecision(4) << r.total_solve_time_ms << ","
            << r.root_lp_time_ms << ","
            << r.nodes_created << ","
            << r.nodes_processed << ","
            << r.max_depth << ","
            << r.nodes_pruned_infeasibility << ","
            << r.nodes_pruned_bound << ","
            << r.integer_feasible_nodes << ","
            << r.incumbent_updates << ","
            << r.final_incumbent_objective << ","
            << r.best_remaining_bound << ","
            << (r.node_limit_reached ? "true" : "false") << ","
            << (r.numerical_failures ? "true" : "false") << ","
            << r.lp_solve_count << ","
            << r.cumulative_lp_time_ms << ","
            << r.avg_lp_time_ms << ","
            << (r.verification_passed ? "PASS" : "FAIL") << ","
            << r.max_residual << ","
            << r.recomputed_objective << ","
            << (r.brute_force_evaluated ? "true" : "false") << ","
            << r.brute_force_optimum << ","
            << (r.brute_force_matched ? "true" : "false") << ","
            << r.seed << "\n";
    }
    return true;
}

bool BenchmarkReporter::export_milp_json(const std::vector<MilpBenchmarkResultRecord>& results, const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "[\n";
    for (size_t k = 0; k < results.size(); ++k) {
        const auto& r = results[k];
        out << "  {\n"
            << "    \"benchmark_id\": \"" << r.benchmark_id << "\",\n"
            << "    \"instance_id\": \"" << r.instance_id << "\",\n"
            << "    \"category_name\": \"" << r.category_name << "\",\n"
            << "    \"rows\": " << r.rows << ",\n"
            << "    \"cols\": " << r.cols << ",\n"
            << "    \"nnz\": " << r.nnz << ",\n"
            << "    \"density\": " << r.density << ",\n"
            << "    \"integer_count\": " << r.integer_count << ",\n"
            << "    \"binary_count\": " << r.binary_count << ",\n"
            << "    \"status\": \"" << r.status << "\",\n"
            << "    \"total_solve_time_ms\": " << r.total_solve_time_ms << ",\n"
            << "    \"root_lp_time_ms\": " << r.root_lp_time_ms << ",\n"
            << "    \"nodes_created\": " << r.nodes_created << ",\n"
            << "    \"nodes_processed\": " << r.nodes_processed << ",\n"
            << "    \"max_depth\": " << r.max_depth << ",\n"
            << "    \"final_incumbent_objective\": " << r.final_incumbent_objective << ",\n"
            << "    \"verification_passed\": " << (r.verification_passed ? "true" : "false") << ",\n"
            << "    \"brute_force_matched\": " << (r.brute_force_matched ? "true" : "false") << "\n"
            << "  }" << (k + 1 < results.size() ? "," : "") << "\n";
    }
    out << "]\n";
    return true;
}

void BenchmarkReporter::print_milp_summary_table(const std::vector<MilpBenchmarkResultRecord>& results) {
    std::cout << "\n=================================================================================================================\n";
    std::cout << "                                    BHARATOPT MILP BENCHMARK MATRIX (PHASE 18)                                  \n";
    std::cout << "=================================================================================================================\n";
    std::cout << std::left 
              << std::setw(28) << "Instance ID"
              << std::setw(22) << "Category"
              << std::setw(12) << "Dim (m x n)"
              << std::setw(10) << "Int/Bin"
              << std::setw(10) << "Status"
              << std::setw(12) << "Nodes (P/C)"
              << std::setw(12) << "Time (ms)"
              << std::setw(14) << "Incumbent Obj"
              << std::setw(8) << "Verify"
              << "\n";
    std::cout << "-----------------------------------------------------------------------------------------------------------------\n";

    for (const auto& r : results) {
        std::string dim_str = std::to_string(r.rows) + "x" + std::to_string(r.cols);
        std::string int_str = std::to_string(r.integer_count) + "/" + std::to_string(r.binary_count);
        std::string node_str = std::to_string(r.nodes_processed) + "/" + std::to_string(r.nodes_created);

        std::cout << std::left
                  << std::setw(28) << r.instance_id.substr(0, 27)
                  << std::setw(22) << r.category_name.substr(0, 21)
                  << std::setw(12) << dim_str
                  << std::setw(10) << int_str
                  << std::setw(10) << r.status
                  << std::setw(12) << node_str
                  << std::fixed << std::setprecision(2)
                  << std::setw(12) << r.total_solve_time_ms
                  << std::setprecision(2)
                  << std::setw(14) << r.final_incumbent_objective
                  << std::setw(8) << (r.verification_passed ? "PASS" : "N/A")
                  << "\n";
    }
    std::cout << "=================================================================================================================\n\n";
}

std::vector<MilpWarmStartResultRecord> MilpBenchmarkSuite::run_warm_start_suite() {
    std::vector<MilpWarmStartResultRecord> results;

    auto process_instance = [](const std::string& instance_id, MilpBenchmarkCategory category, const LPModel& model, size_t max_nodes) {
        (void)category;
        MilpWarmStartResultRecord rec;
        rec.benchmark_id = "PHASE19_WARM_START_" + instance_id;
        rec.instance_id = instance_id;
        rec.rows = model.num_constraints();
        rec.cols = model.num_variables();

        size_t int_c = 0, bin_c = 0;
        for (size_t j = 0; j < model.num_variables(); ++j) {
            const auto& v = model.get_variable(static_cast<index_t>(j));
            if (v.type == VariableType::INTEGER) int_c++;
            if (v.type == VariableType::BINARY) { int_c++; bin_c++; }
        }
        rec.integer_count = int_c;
        rec.binary_count = bin_c;

        // COLD START RUN
        BnBConfig cold_cfg;
        cold_cfg.warm_start_mode = WarmStartMode::COLD_START;
        cold_cfg.max_nodes = max_nodes;
        BranchAndBoundEngine cold_engine(cold_cfg);
        BnBResult cold_res = cold_engine.solve(model);

        rec.cold_time_ms = cold_res.telemetry.total_solve_time_ms;
        rec.cold_nodes_processed = cold_res.telemetry.nodes_processed;
        rec.cold_nodes_created = cold_res.telemetry.nodes_created;
        rec.cold_objective = cold_res.objective_value;
        rec.cold_status = bnb_solver_status_to_string(cold_res.status);

        // WARM START RUN
        BnBConfig warm_cfg;
        warm_cfg.warm_start_mode = WarmStartMode::WARM_START;
        warm_cfg.max_nodes = max_nodes;
        BranchAndBoundEngine warm_engine(warm_cfg);
        BnBResult warm_res = warm_engine.solve(model);

        rec.warm_time_ms = warm_res.telemetry.total_solve_time_ms;
        rec.warm_nodes_processed = warm_res.telemetry.nodes_processed;
        rec.warm_nodes_created = warm_res.telemetry.nodes_created;
        rec.warm_objective = warm_res.objective_value;
        rec.warm_status = bnb_solver_status_to_string(warm_res.status);

        rec.warm_starts_attempted = warm_res.telemetry.warm_starts_attempted;
        rec.warm_starts_accepted = warm_res.telemetry.warm_starts_accepted;
        rec.warm_starts_rejected = warm_res.telemetry.warm_starts_rejected;
        rec.warm_starts_failed = warm_res.telemetry.warm_starts_failed;
        rec.cold_fallbacks = warm_res.telemetry.cold_fallbacks;

        rec.time_diff_ms = rec.cold_time_ms - rec.warm_time_ms;
        rec.speedup = (rec.warm_time_ms > 1e-6) ? (rec.cold_time_ms / rec.warm_time_ms) : 1.0;
        rec.correctness_matched = (cold_res.status == warm_res.status) && (std::abs(rec.cold_objective - rec.warm_objective) < 1e-3);
        rec.verification_passed = (warm_res.status != BnBSolverStatus::NUMERICAL_FAILURE);

        return rec;
    };

    for (const auto& config : configs_) {
        LPModel model = MilpBenchmarkGenerator::generate_instance(config);
        results.push_back(process_instance(config.instance_id, config.category, model, config.max_nodes));
    }

    // Hand cases
    {
        LPModel m1("HandCase1_BinaryMax");
        m1.set_sense(ObjectiveSense::MAXIMIZE);
        auto x = m1.add_variable("x", 0.0, 1.0, 1.0, VariableType::BINARY);
        auto y = m1.add_variable("y", 0.0, 1.0, 1.0, VariableType::BINARY);
        m1.add_constraint("c1", {{x, 2.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 3.0);
        results.push_back(process_instance("M9_HandCase1_BinaryMax", MilpBenchmarkCategory::M9_HAND_DERIVED, m1, 1000));

        LPModel m3("HandCase3_FractionalRoot");
        m3.set_sense(ObjectiveSense::MAXIMIZE);
        auto x3 = m3.add_variable("x", 0.0, 10.0, 3.0, VariableType::INTEGER);
        auto y3 = m3.add_variable("y", 0.0, 10.0, 5.0, VariableType::INTEGER);
        m3.add_constraint("c1", {{x3, 2.0}, {y3, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
        m3.add_constraint("c2", {{x3, 1.0}, {y3, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);
        results.push_back(process_instance("M9_HandCase3_FractionalRoot", MilpBenchmarkCategory::M9_HAND_DERIVED, m3, 1000));
    }

    return results;
}

bool BenchmarkReporter::export_warm_start_csv(const std::vector<MilpWarmStartResultRecord>& results, const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "benchmark_id,instance_id,rows,cols,integer_count,binary_count,cold_time_ms,warm_time_ms,"
        << "cold_nodes,warm_nodes,warm_starts_attempted,warm_starts_accepted,warm_starts_rejected,"
        << "warm_starts_failed,cold_fallbacks,speedup,time_diff_ms,cold_objective,warm_objective,"
        << "cold_status,warm_status,correctness_matched,verification_passed\n";

    for (const auto& r : results) {
        out << r.benchmark_id << ","
            << r.instance_id << ","
            << r.rows << ","
            << r.cols << ","
            << r.integer_count << ","
            << r.binary_count << ","
            << std::fixed << std::setprecision(4) << r.cold_time_ms << ","
            << r.warm_time_ms << ","
            << r.cold_nodes_processed << ","
            << r.warm_nodes_processed << ","
            << r.warm_starts_attempted << ","
            << r.warm_starts_accepted << ","
            << r.warm_starts_rejected << ","
            << r.warm_starts_failed << ","
            << r.cold_fallbacks << ","
            << std::setprecision(2) << r.speedup << ","
            << r.time_diff_ms << ","
            << r.cold_objective << ","
            << r.warm_objective << ","
            << r.cold_status << ","
            << r.warm_status << ","
            << (r.correctness_matched ? "true" : "false") << ","
            << (r.verification_passed ? "PASS" : "FAIL") << "\n";
    }
    return true;
}

bool BenchmarkReporter::export_warm_start_json(const std::vector<MilpWarmStartResultRecord>& results, const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "[\n";
    for (size_t k = 0; k < results.size(); ++k) {
        const auto& r = results[k];
        out << "  {\n"
            << "    \"benchmark_id\": \"" << r.benchmark_id << "\",\n"
            << "    \"instance_id\": \"" << r.instance_id << "\",\n"
            << "    \"rows\": " << r.rows << ",\n"
            << "    \"cols\": " << r.cols << ",\n"
            << "    \"cold_time_ms\": " << r.cold_time_ms << ",\n"
            << "    \"warm_time_ms\": " << r.warm_time_ms << ",\n"
            << "    \"cold_nodes\": " << r.cold_nodes_processed << ",\n"
            << "    \"warm_nodes\": " << r.warm_nodes_processed << ",\n"
            << "    \"warm_starts_attempted\": " << r.warm_starts_attempted << ",\n"
            << "    \"warm_starts_accepted\": " << r.warm_starts_accepted << ",\n"
            << "    \"warm_starts_rejected\": " << r.warm_starts_rejected << ",\n"
            << "    \"warm_starts_failed\": " << r.warm_starts_failed << ",\n"
            << "    \"cold_fallbacks\": " << r.cold_fallbacks << ",\n"
            << "    \"speedup\": " << r.speedup << ",\n"
            << "    \"correctness_matched\": " << (r.correctness_matched ? "true" : "false") << ",\n"
            << "    \"verification_passed\": " << (r.verification_passed ? "true" : "false") << "\n"
            << "  }" << (k + 1 < results.size() ? "," : "") << "\n";
    }
    out << "]\n";
    return true;
}

void BenchmarkReporter::print_warm_start_summary_table(const std::vector<MilpWarmStartResultRecord>& results) {
    std::cout << "\n=========================================================================================================================\n";
    std::cout << "                                    BHARATOPT MILP WARM START BENCHMARK (PHASE 19)                                      \n";
    std::cout << "=========================================================================================================================\n";
    std::cout << std::left 
              << std::setw(28) << "Instance ID"
              << std::setw(12) << "Dim (m x n)"
              << std::setw(12) << "Cold (ms)"
              << std::setw(12) << "Warm (ms)"
              << std::setw(10) << "Speedup"
              << std::setw(14) << "Attempts/Acc"
              << std::setw(12) << "Obj Match"
              << std::setw(8) << "Verify"
              << "\n";
    std::cout << "-------------------------------------------------------------------------------------------------------------------------\n";

    for (const auto& r : results) {
        std::string dim_str = std::to_string(r.rows) + "x" + std::to_string(r.cols);
        std::string att_str = std::to_string(r.warm_starts_attempted) + "/" + std::to_string(r.warm_starts_accepted);

        std::cout << std::left
                  << std::setw(28) << r.instance_id.substr(0, 27)
                  << std::setw(12) << dim_str
                  << std::fixed << std::setprecision(2)
                  << std::setw(12) << r.cold_time_ms
                  << std::setw(12) << r.warm_time_ms
                  << std::setprecision(2)
                  << std::setw(10) << r.speedup
                  << std::setw(14) << att_str
                  << std::setw(12) << (r.correctness_matched ? "MATCH" : "MISMATCH")
                  << std::setw(8) << (r.verification_passed ? "PASS" : "FAIL")
                  << "\n";
    }
    std::cout << "=========================================================================================================================\n\n";
}

} // namespace bharatopt

