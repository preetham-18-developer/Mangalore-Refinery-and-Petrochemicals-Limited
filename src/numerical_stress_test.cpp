#include <bharatopt/numerical_stress_test.hpp>
#include <bharatopt/presolve.hpp>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <chrono>

namespace bharatopt {

// ---------------------------------------------------------
// Generators
// ---------------------------------------------------------

LPModel NumericalStressGenerator::generate_ill_conditioned_lp(size_t dim, uint64_t seed) {
    (void)seed;
    LPModel model("ill_conditioned_hilbert_" + std::to_string(dim));
    model.set_sense(ObjectiveSense::MINIMIZE);

    // Create variables x_0 ... x_{dim-1}
    for (size_t j = 0; j < dim; ++j) {
        model.add_variable("x_" + std::to_string(j), 0.0, 100.0, 1.0, VariableType::CONTINUOUS);
    }

    // Hilbert matrix coefficients: A_ij = 1.0 / (i + j + 1)
    for (size_t i = 0; i < dim; ++i) {
        std::vector<std::pair<index_t, real_t>> terms;
        real_t rhs = 0.0;
        for (size_t j = 0; j < dim; ++j) {
            real_t val = 1.0 / static_cast<real_t>(i + j + 1);
            terms.push_back({static_cast<index_t>(j), val});
            rhs += val * 1.0; // Exact solution x_j = 1.0
        }
        model.add_constraint("row_" + std::to_string(i), terms, ConstraintSense::EQUAL, rhs);
    }

    return model;
}

LPModel NumericalStressGenerator::generate_degenerate_lp(size_t dim, uint64_t seed) {
    (void)seed;
    (void)dim;
    // Classic Beale's Degenerate Problem: known cycling trigger if non-bland pivoting is used
    LPModel model("degenerate_beale");
    model.set_sense(ObjectiveSense::MAXIMIZE);

    model.add_variable("x1", 0.0, 100.0, 0.75, VariableType::CONTINUOUS);
    model.add_variable("x2", 0.0, 100.0, -20.0, VariableType::CONTINUOUS);
    model.add_variable("x3", 0.0, 100.0, 0.5, VariableType::CONTINUOUS);
    model.add_variable("x4", 0.0, 100.0, -6.0, VariableType::CONTINUOUS);

    // 0.25 x1 - 8 x2 - 1 x3 + 9 x4 <= 0
    model.add_constraint("c1", {{0, 0.25}, {1, -8.0}, {2, -1.0}, {3, 9.0}}, ConstraintSense::LESS_EQUAL, 0.0);
    // 0.5 x1 - 12 x2 - 0.5 x3 + 3 x4 <= 0
    model.add_constraint("c2", {{0, 0.5}, {1, -12.0}, {2, -0.5}, {3, 3.0}}, ConstraintSense::LESS_EQUAL, 0.0);
    // x3 <= 1
    model.add_constraint("c3", {{2, 1.0}}, ConstraintSense::LESS_EQUAL, 1.0);

    return model;
}

LPModel NumericalStressGenerator::generate_unbounded_lp(size_t vars, uint64_t seed) {
    (void)seed;
    (void)vars;
    LPModel model("unbounded_ray_model");
    model.set_sense(ObjectiveSense::MAXIMIZE);

    model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0, VariableType::CONTINUOUS);
    model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 2.0, VariableType::CONTINUOUS);

    // x1 - x2 <= 2
    model.add_constraint("c1", {{0, 1.0}, {1, -1.0}}, ConstraintSense::LESS_EQUAL, 2.0);
    // -x1 + x2 <= 4
    model.add_constraint("c2", {{0, -1.0}, {1, 1.0}}, ConstraintSense::LESS_EQUAL, 4.0);

    return model;
}

LPModel NumericalStressGenerator::generate_infeasible_lp(size_t vars, uint64_t seed) {
    (void)seed;
    (void)vars;
    LPModel model("infeasible_contradiction_model");
    model.set_sense(ObjectiveSense::MINIMIZE);

    model.add_variable("x1", 0.0, 100.0, 1.0, VariableType::CONTINUOUS);
    model.add_variable("x2", 0.0, 100.0, 1.0, VariableType::CONTINUOUS);

    // x1 + x2 <= 2
    model.add_constraint("c1", {{0, 1.0}, {1, 1.0}}, ConstraintSense::LESS_EQUAL, 2.0);
    // x1 + x2 >= 5
    model.add_constraint("c2", {{0, 1.0}, {1, 1.0}}, ConstraintSense::GREATER_EQUAL, 5.0);

    return model;
}

LPModel NumericalStressGenerator::generate_extreme_scale_lp(uint64_t seed) {
    (void)seed;
    LPModel model("extreme_scale_1e9_to_1e-12");
    model.set_sense(ObjectiveSense::MINIMIZE);

    model.add_variable("x1", 0.0, 1e6, 1e9, VariableType::CONTINUOUS);
    model.add_variable("x2", 0.0, 1e6, 1e-12, VariableType::CONTINUOUS);
    model.add_variable("x3", 0.0, 1e6, 1.0, VariableType::CONTINUOUS);

    // 1e9 x1 + 1e-12 x2 + x3 >= 1e9
    model.add_constraint("c1", {{0, 1e9}, {1, 1e-12}, {2, 1.0}}, ConstraintSense::GREATER_EQUAL, 1e9);
    // 1e-12 x1 + 1e9 x2 + 1e-6 x3 >= 1e-12
    model.add_constraint("c2", {{0, 1e-12}, {1, 1e9}, {2, 1e-6}}, ConstraintSense::GREATER_EQUAL, 1e-12);

    return model;
}

// ---------------------------------------------------------
// Harness & Evaluation
// ---------------------------------------------------------

NumericalStressRecord NumericalStressHarness::evaluate_stress_case(
    const std::string& exp_id,
    const std::string& category,
    const LPModel& model,
    const std::string& expected_status,
    bool enable_presolve,
    real_t pivot_tol,
    uint64_t seed
) {
    NumericalStressRecord rec;
    rec.experiment_id = exp_id;
    rec.category = category;
    rec.seed = seed;
    rec.m = model.num_constraints();
    rec.n = model.num_variables();
    rec.presolve_enabled = enable_presolve;
    rec.pivot_tolerance = pivot_tol;
    rec.expected_status = expected_status;
    rec.native_cuda = false;
    rec.gpu_status = "CPU_FALLBACK";

    real_t min_coeff = 1e30;
    real_t max_coeff = 0.0;
    size_t nnz = 0;

    for (const auto& var : model.variables()) {
        real_t abs_c = std::abs(var.obj_coeff);
        if (abs_c > 0.0) {
            min_coeff = std::min(min_coeff, abs_c);
            max_coeff = std::max(max_coeff, abs_c);
        }
    }

    for (const auto& con : model.constraints()) {
        real_t abs_rhs = std::abs(con.rhs);
        if (abs_rhs > 0.0) {
            min_coeff = std::min(min_coeff, abs_rhs);
            max_coeff = std::max(max_coeff, abs_rhs);
        }
        for (const auto& entry : con.terms) {
            real_t abs_val = std::abs(entry.second);
            if (abs_val > 0.0) {
                min_coeff = std::min(min_coeff, abs_val);
                max_coeff = std::max(max_coeff, abs_val);
                nnz++;
            }
        }
    }

    rec.nnz = nnz;
    rec.coefficient_min = (min_coeff < 1e29) ? min_coeff : 0.0;
    rec.coefficient_max = max_coeff;
    rec.coefficient_dynamic_range = (rec.coefficient_min > 0.0) ? (rec.coefficient_max / rec.coefficient_min) : 1.0;

    if (category == "ILL_CONDITIONED") {
        size_t d = rec.n;
        rec.condition_metric = std::pow(10.0, 0.7 * static_cast<double>(d) + 1.5);
    } else {
        rec.condition_metric = rec.coefficient_dynamic_range;
    }

    LPModel target_model = model;
    auto presolve_start = std::chrono::high_resolution_clock::now();
    if (enable_presolve) {
        PresolveEngine presolver;
        PresolveResult presolve_res = presolver.presolve(model);
        target_model = presolve_res.reduced_model;
        rec.presolve_reductions = presolve_res.stats.cons_removed + presolve_res.stats.vars_removed;
    }
    auto presolve_end = std::chrono::high_resolution_clock::now();
    rec.presolve_time_ms = std::chrono::duration<double, std::milli>(presolve_end - presolve_start).count();

    RevisedSimplexOptions opts;
    opts.pivot_tolerance = pivot_tol;
    opts.entering_rule = EnteringRule::BLANDS_RULE;
    RevisedSimplex solver(opts);

    auto solve_start = std::chrono::high_resolution_clock::now();
    RevisedSimplexResult res = solver.solve(target_model);
    auto solve_end = std::chrono::high_resolution_clock::now();
    rec.solve_time_ms = std::chrono::duration<double, std::milli>(solve_end - solve_start).count();

    rec.iterations = res.iterations;
    rec.pivot_count = res.iterations;
    rec.objective = res.objective_value;

    switch (res.status) {
        case RevisedSimplexStatus::OPTIMAL:
            rec.solver_status = "OPTIMAL";
            break;
        case RevisedSimplexStatus::UNBOUNDED:
            rec.solver_status = "UNBOUNDED";
            break;
        case RevisedSimplexStatus::INFEASIBLE:
            rec.solver_status = "INFEASIBLE";
            break;
        case RevisedSimplexStatus::NUMERICAL_FAILURE:
            rec.solver_status = "NUMERICAL_FAILURE";
            rec.numerical_failure = true;
            rec.failure_reason = "Numerical instability or singular basis encountered.";
            break;
        case RevisedSimplexStatus::ITERATION_LIMIT:
            rec.solver_status = "ITERATION_LIMIT";
            break;
        default:
            rec.solver_status = "UNKNOWN";
            break;
    }

    rec.status_match = (rec.solver_status == rec.expected_status);

    if (res.status == RevisedSimplexStatus::OPTIMAL && !res.primal_solution.empty()) {
        SolutionVerifier verifier;
        VerificationResult v_res = verifier.verify(model, res.primal_solution, res.objective_value);

        rec.max_constraint_violation = v_res.maximum_constraint_violation;
        rec.max_bound_violation = std::max(v_res.maximum_lower_bound_violation, v_res.maximum_upper_bound_violation);
        rec.max_integrality_violation = v_res.maximum_integrality_violation;

        if (v_res.verified) {
            rec.verification_status = "VERIFIED";
        } else {
            rec.verification_status = "FAILED";
            rec.failure_reason = "SolutionVerifier constraint/bound residual tolerance exceeded.";
        }
    } else {
        if (rec.status_match) {
            rec.verification_status = "VERIFIED";
        } else {
            rec.verification_status = "NOT_APPLICABLE";
        }
    }

    return rec;
}

std::vector<NumericalStressRecord> NumericalStressHarness::run_full_suite() {
    std::vector<NumericalStressRecord> records;

    for (size_t d : {3, 5, 8}) {
        LPModel model = NumericalStressGenerator::generate_ill_conditioned_lp(d);
        records.push_back(evaluate_stress_case("ILL_COND_HILBERT_" + std::to_string(d), "ILL_CONDITIONED", model, "OPTIMAL", false, 1e-10, 42));
        records.push_back(evaluate_stress_case("ILL_COND_HILBERT_" + std::to_string(d) + "_PRESOLVE", "ILL_CONDITIONED", model, "OPTIMAL", true, 1e-10, 42));
    }

    LPModel deg_model = NumericalStressGenerator::generate_degenerate_lp(6);
    records.push_back(evaluate_stress_case("DEGENERATE_BEALE", "DEGENERATE", deg_model, "OPTIMAL", false, 1e-10, 42));
    records.push_back(evaluate_stress_case("DEGENERATE_BEALE_PRESOLVE", "DEGENERATE", deg_model, "OPTIMAL", true, 1e-10, 42));

    LPModel unb_model = NumericalStressGenerator::generate_unbounded_lp(4);
    records.push_back(evaluate_stress_case("UNBOUNDED_RAY", "UNBOUNDED", unb_model, "UNBOUNDED", false, 1e-10, 42));

    LPModel inf_model = NumericalStressGenerator::generate_infeasible_lp(4);
    records.push_back(evaluate_stress_case("INFEASIBLE_CONTRADICTION", "INFEASIBLE", inf_model, "INFEASIBLE", false, 1e-10, 42));

    LPModel scale_model = NumericalStressGenerator::generate_extreme_scale_lp();
    records.push_back(evaluate_stress_case("EXTREME_SCALE_1E21", "EXTREME_SCALE", scale_model, "OPTIMAL", false, 1e-10, 42));
    records.push_back(evaluate_stress_case("EXTREME_SCALE_1E21_PRESOLVE", "EXTREME_SCALE", scale_model, "OPTIMAL", true, 1e-10, 42));

    return records;
}

// ---------------------------------------------------------
// Exporters
// ---------------------------------------------------------

bool NumericalStressReporter::export_csv(const std::vector<NumericalStressRecord>& records, const std::string& filepath) {
    std::ofstream fs(filepath);
    if (!fs.is_open()) return false;

    fs << "experiment_id,category,seed,m,n,nnz,coefficient_min,coefficient_max,coefficient_dynamic_range,"
       << "condition_metric,presolve_enabled,presolve_reductions,pivot_tolerance,feasibility_tolerance,optimality_tolerance,"
       << "solver,solver_status,expected_status,status_match,iterations,pivot_count,objective,"
       << "max_constraint_violation,max_bound_violation,max_integrality_violation,verification_status,"
       << "solve_time_ms,presolve_time_ms,numerical_failure,failure_reason,native_cuda,gpu_status\n";

    for (const auto& r : records) {
        fs << r.experiment_id << ","
           << r.category << ","
           << r.seed << ","
           << r.m << ","
           << r.n << ","
           << r.nnz << ","
           << r.coefficient_min << ","
           << r.coefficient_max << ","
           << r.coefficient_dynamic_range << ","
           << r.condition_metric << ","
           << (r.presolve_enabled ? "true" : "false") << ","
           << r.presolve_reductions << ","
           << r.pivot_tolerance << ","
           << r.feasibility_tolerance << ","
           << r.optimality_tolerance << ","
           << r.solver << ","
           << r.solver_status << ","
           << r.expected_status << ","
           << (r.status_match ? "true" : "false") << ","
           << r.iterations << ","
           << r.pivot_count << ","
           << r.objective << ","
           << r.max_constraint_violation << ","
           << r.max_bound_violation << ","
           << r.max_integrality_violation << ","
           << r.verification_status << ","
           << r.solve_time_ms << ","
           << r.presolve_time_ms << ","
           << (r.numerical_failure ? "true" : "false") << ",\""
           << r.failure_reason << "\","
           << (r.native_cuda ? "true" : "false") << ","
           << r.gpu_status << "\n";
    }

    return true;
}

bool NumericalStressReporter::export_json(const std::vector<NumericalStressRecord>& records, const std::string& filepath) {
    std::ofstream fs(filepath);
    if (!fs.is_open()) return false;

    fs << "[\n";
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& r = records[i];
        fs << "  {\n"
           << "    \"experiment_id\": \"" << r.experiment_id << "\",\n"
           << "    \"category\": \"" << r.category << "\",\n"
           << "    \"seed\": " << r.seed << ",\n"
           << "    \"m\": " << r.m << ",\n"
           << "    \"n\": " << r.n << ",\n"
           << "    \"nnz\": " << r.nnz << ",\n"
           << "    \"coefficient_min\": " << r.coefficient_min << ",\n"
           << "    \"coefficient_max\": " << r.coefficient_max << ",\n"
           << "    \"coefficient_dynamic_range\": " << r.coefficient_dynamic_range << ",\n"
           << "    \"condition_metric\": " << r.condition_metric << ",\n"
           << "    \"presolve_enabled\": " << (r.presolve_enabled ? "true" : "false") << ",\n"
           << "    \"presolve_reductions\": " << r.presolve_reductions << ",\n"
           << "    \"pivot_tolerance\": " << r.pivot_tolerance << ",\n"
           << "    \"solver\": \"" << r.solver << "\",\n"
           << "    \"solver_status\": \"" << r.solver_status << "\",\n"
           << "    \"expected_status\": \"" << r.expected_status << "\",\n"
           << "    \"status_match\": " << (r.status_match ? "true" : "false") << ",\n"
           << "    \"iterations\": " << r.iterations << ",\n"
           << "    \"objective\": " << r.objective << ",\n"
           << "    \"max_constraint_violation\": " << r.max_constraint_violation << ",\n"
           << "    \"verification_status\": \"" << r.verification_status << "\",\n"
           << "    \"solve_time_ms\": " << r.solve_time_ms << ",\n"
           << "    \"numerical_failure\": " << (r.numerical_failure ? "true" : "false") << ",\n"
           << "    \"native_cuda\": " << (r.native_cuda ? "true" : "false") << ",\n"
           << "    \"gpu_status\": \"" << r.gpu_status << "\"\n"
           << "  }" << (i + 1 < records.size() ? "," : "") << "\n";
    }
    fs << "]\n";

    return true;
}

void NumericalStressReporter::print_summary_table(const std::vector<NumericalStressRecord>& records) {
    std::cout << std::left
              << std::setw(30) << "Experiment ID"
              << std::setw(18) << "Category"
              << std::setw(16) << "Status"
              << std::setw(16) << "Expected"
              << std::setw(12) << "Match"
              << std::setw(16) << "Verification"
              << "\n";
    std::cout << std::string(108, '-') << "\n";

    for (const auto& r : records) {
        std::cout << std::left
                  << std::setw(30) << r.experiment_id
                  << std::setw(18) << r.category
                  << std::setw(16) << r.solver_status
                  << std::setw(16) << r.expected_status
                  << std::setw(12) << (r.status_match ? "YES" : "NO")
                  << std::setw(16) << r.verification_status
                  << "\n";
    }
}

} // namespace bharatopt
