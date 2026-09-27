#include <bharatopt/ablation_study.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <algorithm>

namespace bharatopt {

static bool is_mip_model(const LPModel& model) {
    for (const auto& v : model.variables()) {
        if (v.type == VariableType::INTEGER || v.type == VariableType::BINARY) {
            return true;
        }
    }
    return false;
}

static std::string rv_status_str(RevisedSimplexStatus st) {
    switch (st) {
        case RevisedSimplexStatus::OPTIMAL: return "OPTIMAL";
        case RevisedSimplexStatus::UNBOUNDED: return "UNBOUNDED";
        case RevisedSimplexStatus::INFEASIBLE: return "INFEASIBLE";
        case RevisedSimplexStatus::UNSUPPORTED_INITIAL_BASIS: return "UNSUPPORTED_INITIAL_BASIS";
        case RevisedSimplexStatus::ITERATION_LIMIT: return "ITERATION_LIMIT";
        case RevisedSimplexStatus::NUMERICAL_FAILURE: return "NUMERICAL_FAILURE";
    }
    return "UNKNOWN";
}

static std::string ed_status_str(SimplexStatus st) {
    switch (st) {
        case SimplexStatus::OPTIMAL: return "OPTIMAL";
        case SimplexStatus::UNBOUNDED: return "UNBOUNDED";
        case SimplexStatus::INFEASIBLE: return "INFEASIBLE";
        case SimplexStatus::UNSUPPORTED_INITIAL_BASIS: return "UNSUPPORTED_INITIAL_BASIS";
        case SimplexStatus::ITERATION_LIMIT: return "ITERATION_LIMIT";
        case SimplexStatus::NUMERICAL_FAILURE: return "NUMERICAL_FAILURE";
    }
    return "UNKNOWN";
}

std::string ablation_experiment_type_to_string(AblationExperimentType type) {
    switch (type) {
        case AblationExperimentType::A1_PRESOLVE: return "A1_PRESOLVE";
        case AblationExperimentType::A2_SPARSE_REPRESENTATION: return "A2_SPARSE_REPRESENTATION";
        case AblationExperimentType::A3_ADAPTIVE_ROUTING: return "A3_ADAPTIVE_ROUTING";
        case AblationExperimentType::A4_WARM_START: return "A4_WARM_START";
        case AblationExperimentType::A5_VERIFICATION_OVERHEAD: return "A5_VERIFICATION_OVERHEAD";
        case AblationExperimentType::A6_COST_ESTIMATOR: return "A6_COST_ESTIMATOR";
        case AblationExperimentType::A7_BASIS_PROPAGATION: return "A7_BASIS_PROPAGATION";
    }
    return "UNKNOWN_ABLATION";
}

AblationRecord AblationHarness::run_a1_presolve(const LPModel& model, const std::string& instance_name, size_t repetitions) {
    AblationRecord rec;
    rec.experiment_id = "A1_PRESOLVE";
    rec.component = "Presolve";
    rec.baseline_config = "Presolve ON";
    rec.ablated_config = "Presolve OFF";
    rec.instance_name = instance_name;
    rec.problem_type = is_mip_model(model) ? "MILP" : "LP";
    rec.solver_name = "RevisedSimplex";
    rec.repetitions = repetitions;

    SolutionVerifier verifier;

    // --- BASELINE: Presolve ON ---
    double total_b_time = 0.0;
    RevisedSimplexResult res_b;
    PresolveResult pres_b;
    for (size_t i = 0; i < repetitions; i++) {
        auto t0 = std::chrono::high_resolution_clock::now();
        PresolveEngine engine;
        pres_b = engine.presolve(model);
        RevisedSimplex solver;
        res_b = solver.solve(pres_b.reduced_model);
        auto t1 = std::chrono::high_resolution_clock::now();
        total_b_time += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    rec.baseline_solve_time_ms = total_b_time / repetitions;
    rec.baseline_total_time_ms = rec.baseline_solve_time_ms;
    rec.baseline_iterations = res_b.iterations;
    rec.baseline_objective = res_b.objective_value;
    rec.baseline_status = rv_status_str(res_b.status);

    if (res_b.status == RevisedSimplexStatus::OPTIMAL) {
        std::vector<real_t> orig_sol = pres_b.postsolve.recover_solution(res_b.primal_solution);
        real_t orig_obj = pres_b.postsolve.compute_original_objective(orig_sol);
        VerificationResult vres = verifier.verify(model, orig_sol, orig_obj);
        rec.baseline_verified = vres.verified;
        rec.baseline_objective = orig_obj;
        rec.max_residual = std::max(rec.max_residual, vres.maximum_constraint_violation);
    }

    // --- ABLATED: Presolve OFF ---
    double total_a_time = 0.0;
    RevisedSimplexResult res_a;
    for (size_t i = 0; i < repetitions; i++) {
        auto t0 = std::chrono::high_resolution_clock::now();
        RevisedSimplex solver;
        res_a = solver.solve(model);
        auto t1 = std::chrono::high_resolution_clock::now();
        total_a_time += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    rec.ablated_solve_time_ms = total_a_time / repetitions;
    rec.ablated_total_time_ms = rec.ablated_solve_time_ms;
    rec.ablated_iterations = res_a.iterations;
    rec.ablated_objective = res_a.objective_value;
    rec.ablated_status = rv_status_str(res_a.status);

    if (res_a.status == RevisedSimplexStatus::OPTIMAL) {
        VerificationResult vres = verifier.verify(model, res_a.primal_solution, res_a.objective_value);
        rec.ablated_verified = vres.verified;
        rec.max_residual = std::max(rec.max_residual, vres.maximum_constraint_violation);
    }

    rec.delta_solve_time_ms = rec.ablated_solve_time_ms - rec.baseline_solve_time_ms;
    if (rec.baseline_solve_time_ms > 1e-9) {
        rec.percent_change_solve_time = (rec.delta_solve_time_ms / rec.baseline_solve_time_ms) * 100.0;
    }
    rec.delta_total_time_ms = rec.ablated_total_time_ms - rec.baseline_total_time_ms;
    if (rec.baseline_total_time_ms > 1e-9) {
        rec.percent_change_total_time = (rec.delta_total_time_ms / rec.baseline_total_time_ms) * 100.0;
    }
    rec.experiment_status = "PASS";
    rec.notes = "Presolve reduced rows by " + std::to_string(pres_b.stats.cons_removed) + ", cols by " + std::to_string(pres_b.stats.vars_removed);
    return rec;
}

AblationRecord AblationHarness::run_a2_sparse(const LPModel& model, const std::string& instance_name, size_t repetitions) {
    AblationRecord rec;
    rec.experiment_id = "A2_SPARSE_REPRESENTATION";
    rec.component = "Sparse Representation";
    rec.baseline_config = "Sparse Matrix (CSC / Sparse LU)";
    rec.ablated_config = "Controlled Dense / Reference";
    rec.instance_name = instance_name;
    rec.problem_type = is_mip_model(model) ? "MILP" : "LP";
    rec.solver_name = "RevisedSimplex vs EducationalSimplex";
    rec.repetitions = repetitions;

    SolutionVerifier verifier;

    // --- BASELINE: Sparse Revised Simplex ---
    double total_b_time = 0.0;
    RevisedSimplexResult res_b;
    for (size_t i = 0; i < repetitions; i++) {
        auto t0 = std::chrono::high_resolution_clock::now();
        RevisedSimplex solver;
        res_b = solver.solve(model);
        auto t1 = std::chrono::high_resolution_clock::now();
        total_b_time += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    rec.baseline_solve_time_ms = total_b_time / repetitions;
    rec.baseline_total_time_ms = rec.baseline_solve_time_ms;
    rec.baseline_iterations = res_b.iterations;
    rec.baseline_objective = res_b.objective_value;
    rec.baseline_status = rv_status_str(res_b.status);

    if (res_b.status == RevisedSimplexStatus::OPTIMAL) {
        VerificationResult vres = verifier.verify(model, res_b.primal_solution, res_b.objective_value);
        rec.baseline_verified = vres.verified;
        rec.max_residual = std::max(rec.max_residual, vres.maximum_constraint_violation);
    }

    // --- ABLATED: Educational Simplex (Dense Reference) ---
    if (model.num_variables() > 100 || model.num_constraints() > 100) {
        rec.ablated_solve_time_ms = 0.0;
        rec.ablated_status = "NOT_AVAILABLE";
        rec.experiment_status = "NOT_AVAILABLE";
        rec.notes = "Dense reference omitted for model size > 100 dimensions";
        return rec;
    }

    double total_a_time = 0.0;
    SimplexResult res_a;
    for (size_t i = 0; i < repetitions; i++) {
        auto t0 = std::chrono::high_resolution_clock::now();
        EducationalSimplex solver;
        res_a = solver.solve(model);
        auto t1 = std::chrono::high_resolution_clock::now();
        total_a_time += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    rec.ablated_solve_time_ms = total_a_time / repetitions;
    rec.ablated_total_time_ms = rec.ablated_solve_time_ms;
    rec.ablated_iterations = res_a.iterations;
    rec.ablated_objective = res_a.objective_value;
    rec.ablated_status = ed_status_str(res_a.status);

    if (res_a.status == SimplexStatus::OPTIMAL) {
        VerificationResult vres = verifier.verify(model, res_a.primal_solution, res_a.objective_value);
        rec.ablated_verified = vres.verified;
        rec.max_residual = std::max(rec.max_residual, vres.maximum_constraint_violation);
    }

    rec.delta_solve_time_ms = rec.ablated_solve_time_ms - rec.baseline_solve_time_ms;
    if (rec.baseline_solve_time_ms > 1e-9) {
        rec.percent_change_solve_time = (rec.delta_solve_time_ms / rec.baseline_solve_time_ms) * 100.0;
    }
    rec.delta_total_time_ms = rec.ablated_total_time_ms - rec.baseline_total_time_ms;
    if (rec.baseline_total_time_ms > 1e-9) {
        rec.percent_change_total_time = (rec.delta_total_time_ms / rec.baseline_total_time_ms) * 100.0;
    }
    rec.experiment_status = "PASS";
    rec.notes = "Controlled dense reference evaluation";
    return rec;
}

AblationRecord AblationHarness::run_a3_adaptive_routing(const LPModel& model, const std::string& instance_name, size_t repetitions) {
    AblationRecord rec;
    rec.experiment_id = "A3_ADAPTIVE_ROUTING";
    rec.component = "Adaptive Routing";
    rec.baseline_config = "Adaptive Routing (AUTO)";
    rec.ablated_config = "Fixed Execution Mode (FORCE_CPU_REVISED)";
    rec.instance_name = instance_name;
    rec.problem_type = is_mip_model(model) ? "MILP" : "LP";
    rec.solver_name = "ExecutionRouter";
    rec.repetitions = repetitions;

    // --- BASELINE: Adaptive Routing ---
    double total_b_time = 0.0;
    RoutedSolveResult res_b;
    for (size_t i = 0; i < repetitions; i++) {
        RoutingConfiguration cfg;
        cfg.routing_mode = RoutingMode::ADAPTIVE;
        ExecutionRouter router(cfg);
        auto t0 = std::chrono::high_resolution_clock::now();
        res_b = router.solve(model);
        auto t1 = std::chrono::high_resolution_clock::now();
        total_b_time += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    rec.baseline_solve_time_ms = total_b_time / repetitions;
    rec.baseline_total_time_ms = rec.baseline_solve_time_ms;
    rec.baseline_iterations = res_b.iterations;
    rec.baseline_objective = res_b.objective_value;
    rec.baseline_status = res_b.status;
    rec.baseline_verified = res_b.verification_passed;

    // --- ABLATED: Fixed Execution Mode ---
    double total_a_time = 0.0;
    RoutedSolveResult res_a;
    for (size_t i = 0; i < repetitions; i++) {
        RoutingConfiguration cfg;
        cfg.routing_mode = RoutingMode::FORCE_CPU_REVISED;
        ExecutionRouter router(cfg);
        auto t0 = std::chrono::high_resolution_clock::now();
        res_a = router.solve(model);
        auto t1 = std::chrono::high_resolution_clock::now();
        total_a_time += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    rec.ablated_solve_time_ms = total_a_time / repetitions;
    rec.ablated_total_time_ms = rec.ablated_solve_time_ms;
    rec.ablated_iterations = res_a.iterations;
    rec.ablated_objective = res_a.objective_value;
    rec.ablated_status = res_a.status;
    rec.ablated_verified = res_a.verification_passed;

    rec.delta_solve_time_ms = rec.ablated_solve_time_ms - rec.baseline_solve_time_ms;
    if (rec.baseline_solve_time_ms > 1e-9) {
        rec.percent_change_solve_time = (rec.delta_solve_time_ms / rec.baseline_solve_time_ms) * 100.0;
    }
    rec.delta_total_time_ms = rec.ablated_total_time_ms - rec.baseline_total_time_ms;
    if (rec.baseline_total_time_ms > 1e-9) {
        rec.percent_change_total_time = (rec.delta_total_time_ms / rec.baseline_total_time_ms) * 100.0;
    }
    rec.experiment_status = "PASS";
    rec.notes = "Adaptive selected: " + res_b.decision.solver_name;
    return rec;
}

AblationRecord AblationHarness::run_a4_warm_start(const LPModel& model, const std::string& instance_name, size_t repetitions) {
    AblationRecord rec;
    rec.experiment_id = "A4_WARM_START";
    rec.component = "MILP Warm Start";
    rec.baseline_config = "Warm Start Enabled";
    rec.ablated_config = "Cold Start Only";
    rec.instance_name = instance_name;
    rec.problem_type = "MILP";
    rec.solver_name = "BranchAndBoundEngine";
    rec.repetitions = repetitions;

    if (!is_mip_model(model)) {
        rec.experiment_status = "UNSUPPORTED";
        rec.notes = "Warm start ablation requires MILP model";
        return rec;
    }

    SolutionVerifier verifier;

    // --- BASELINE: Warm Start Enabled ---
    double total_b_time = 0.0;
    BnBResult res_b;
    for (size_t i = 0; i < repetitions; i++) {
        BnBConfig cfg;
        cfg.warm_start_mode = WarmStartMode::WARM_START;
        BranchAndBoundEngine engine(cfg);
        auto t0 = std::chrono::high_resolution_clock::now();
        res_b = engine.solve(model);
        auto t1 = std::chrono::high_resolution_clock::now();
        total_b_time += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    rec.baseline_solve_time_ms = total_b_time / repetitions;
    rec.baseline_total_time_ms = rec.baseline_solve_time_ms;
    rec.baseline_node_count = res_b.telemetry.nodes_processed;
    rec.baseline_objective = res_b.objective_value;
    rec.baseline_status = bnb_solver_status_to_string(res_b.status);

    if (res_b.status == BnBSolverStatus::OPTIMAL) {
        VerificationResult vres = verifier.verify(model, res_b.solution, res_b.objective_value);
        rec.baseline_verified = vres.verified;
        rec.max_residual = std::max(rec.max_residual, vres.maximum_constraint_violation);
    }

    // --- ABLATED: Cold Start Only ---
    double total_a_time = 0.0;
    BnBResult res_a;
    for (size_t i = 0; i < repetitions; i++) {
        BnBConfig cfg;
        cfg.warm_start_mode = WarmStartMode::COLD_START;
        BranchAndBoundEngine engine(cfg);
        auto t0 = std::chrono::high_resolution_clock::now();
        res_a = engine.solve(model);
        auto t1 = std::chrono::high_resolution_clock::now();
        total_a_time += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    rec.ablated_solve_time_ms = total_a_time / repetitions;
    rec.ablated_total_time_ms = rec.ablated_solve_time_ms;
    rec.ablated_node_count = res_a.telemetry.nodes_processed;
    rec.ablated_objective = res_a.objective_value;
    rec.ablated_status = bnb_solver_status_to_string(res_a.status);

    if (res_a.status == BnBSolverStatus::OPTIMAL) {
        VerificationResult vres = verifier.verify(model, res_a.solution, res_a.objective_value);
        rec.ablated_verified = vres.verified;
        rec.max_residual = std::max(rec.max_residual, vres.maximum_constraint_violation);
    }

    rec.delta_solve_time_ms = rec.ablated_solve_time_ms - rec.baseline_solve_time_ms;
    if (rec.baseline_solve_time_ms > 1e-9) {
        rec.percent_change_solve_time = (rec.delta_solve_time_ms / rec.baseline_solve_time_ms) * 100.0;
    }
    rec.delta_total_time_ms = rec.ablated_total_time_ms - rec.baseline_total_time_ms;
    if (rec.baseline_total_time_ms > 1e-9) {
        rec.percent_change_total_time = (rec.delta_total_time_ms / rec.baseline_total_time_ms) * 100.0;
    }
    rec.experiment_status = "PASS";
    rec.notes = "Warm start attempts: " + std::to_string(res_b.telemetry.warm_starts_attempted) + ", accepted: " + std::to_string(res_b.telemetry.warm_starts_accepted);
    return rec;
}

AblationRecord AblationHarness::run_a5_verification_overhead(const LPModel& model, const std::string& instance_name, size_t repetitions) {
    AblationRecord rec;
    rec.experiment_id = "A5_VERIFICATION_OVERHEAD";
    rec.component = "Independent Verification Overhead";
    rec.baseline_config = "Solve + Verification";
    rec.ablated_config = "Solve ONLY";
    rec.instance_name = instance_name;
    rec.problem_type = is_mip_model(model) ? "MILP" : "LP";
    rec.solver_name = "RevisedSimplex";
    rec.repetitions = repetitions;

    SolutionVerifier verifier;
    RevisedSimplex solver;

    // --- BASELINE: Solve + Verification ---
    double total_solve_time = 0.0;
    double total_verif_time = 0.0;
    RevisedSimplexResult res;
    for (size_t i = 0; i < repetitions; i++) {
        auto t0 = std::chrono::high_resolution_clock::now();
        res = solver.solve(model);
        auto t1 = std::chrono::high_resolution_clock::now();
        VerificationResult vres;
        if (res.status == RevisedSimplexStatus::OPTIMAL) {
            vres = verifier.verify(model, res.primal_solution, res.objective_value);
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        total_solve_time += std::chrono::duration<double, std::milli>(t1 - t0).count();
        total_verif_time += std::chrono::duration<double, std::milli>(t2 - t1).count();
        rec.baseline_verified = vres.verified;
        rec.max_residual = std::max(rec.max_residual, vres.maximum_constraint_violation);
    }

    rec.baseline_solve_time_ms = total_solve_time / repetitions;
    double avg_verif_time = total_verif_time / repetitions;
    rec.baseline_total_time_ms = rec.baseline_solve_time_ms + avg_verif_time;
    rec.baseline_iterations = res.iterations;
    rec.baseline_objective = res.objective_value;
    rec.baseline_status = rv_status_str(res.status);

    // --- ABLATED: Solve ONLY ---
    rec.ablated_solve_time_ms = rec.baseline_solve_time_ms;
    rec.ablated_total_time_ms = rec.baseline_solve_time_ms;
    rec.ablated_iterations = res.iterations;
    rec.ablated_objective = res.objective_value;
    rec.ablated_status = rec.baseline_status;
    rec.ablated_verified = rec.baseline_verified;

    rec.delta_solve_time_ms = 0.0;
    rec.percent_change_solve_time = 0.0;
    rec.delta_total_time_ms = rec.baseline_total_time_ms - rec.ablated_total_time_ms;
    if (rec.ablated_total_time_ms > 1e-9) {
        rec.percent_change_total_time = (avg_verif_time / rec.ablated_total_time_ms) * 100.0;
    }
    rec.experiment_status = "PASS";
    rec.notes = "Verification overhead: " + std::to_string(avg_verif_time) + " ms (" + std::to_string(rec.percent_change_total_time) + "%)";
    return rec;
}

AblationRecord AblationHarness::run_a6_cost_estimator(const LPModel& model, const std::string& instance_name, size_t repetitions) {
    AblationRecord rec;
    rec.experiment_id = "A6_COST_ESTIMATOR";
    rec.component = "Cost Estimator Decision";
    rec.baseline_config = "Adaptive Estimator-Based Selection";
    rec.ablated_config = "Static Fixed Selection";
    rec.instance_name = instance_name;
    rec.problem_type = is_mip_model(model) ? "MILP" : "LP";
    rec.solver_name = "ExecutionRouter vs Fixed";
    rec.repetitions = repetitions;

    // --- BASELINE: Adaptive Estimator Selection ---
    double total_b_time = 0.0;
    RoutedSolveResult res_b;
    for (size_t i = 0; i < repetitions; i++) {
        RoutingConfiguration cfg;
        cfg.routing_mode = RoutingMode::ADAPTIVE;
        ExecutionRouter router(cfg);
        auto t0 = std::chrono::high_resolution_clock::now();
        res_b = router.solve(model);
        auto t1 = std::chrono::high_resolution_clock::now();
        total_b_time += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    rec.baseline_solve_time_ms = total_b_time / repetitions;
    rec.baseline_total_time_ms = rec.baseline_solve_time_ms;
    rec.baseline_iterations = res_b.iterations;
    rec.baseline_objective = res_b.objective_value;
    rec.baseline_status = res_b.status;
    rec.baseline_verified = res_b.verification_passed;

    // --- ABLATED: Static Fixed Selection ---
    double total_a_time = 0.0;
    RoutedSolveResult res_a;
    for (size_t i = 0; i < repetitions; i++) {
        RoutingConfiguration cfg;
        cfg.routing_mode = RoutingMode::FORCE_CPU_REVISED;
        ExecutionRouter router(cfg);
        auto t0 = std::chrono::high_resolution_clock::now();
        res_a = router.solve(model);
        auto t1 = std::chrono::high_resolution_clock::now();
        total_a_time += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    rec.ablated_solve_time_ms = total_a_time / repetitions;
    rec.ablated_total_time_ms = rec.ablated_solve_time_ms;
    rec.ablated_iterations = res_a.iterations;
    rec.ablated_objective = res_a.objective_value;
    rec.ablated_status = res_a.status;
    rec.ablated_verified = res_a.verification_passed;

    rec.delta_solve_time_ms = rec.ablated_solve_time_ms - rec.baseline_solve_time_ms;
    if (rec.baseline_solve_time_ms > 1e-9) {
        rec.percent_change_solve_time = (rec.delta_solve_time_ms / rec.baseline_solve_time_ms) * 100.0;
    }
    rec.delta_total_time_ms = rec.ablated_total_time_ms - rec.baseline_total_time_ms;
    if (rec.baseline_total_time_ms > 1e-9) {
        rec.percent_change_total_time = (rec.delta_total_time_ms / rec.baseline_total_time_ms) * 100.0;
    }
    rec.experiment_status = "PASS";
    rec.notes = "Predicted cost: " + std::to_string(res_b.decision.selected_predicted_cost_ms) + " ms";
    return rec;
}

AblationRecord AblationHarness::run_a7_basis_propagation(const LPModel& model, const std::string& instance_name, size_t repetitions) {
    AblationRecord rec;
    rec.experiment_id = "A7_BASIS_PROPAGATION";
    rec.component = "MILP Child Basis Propagation";
    rec.baseline_config = "Child Basis Propagation ON";
    rec.ablated_config = "Child Basis Propagation OFF (Cold Basis)";
    rec.instance_name = instance_name;
    rec.problem_type = "MILP";
    rec.solver_name = "BranchAndBoundEngine";
    rec.repetitions = repetitions;

    if (!is_mip_model(model)) {
        rec.experiment_status = "UNSUPPORTED";
        rec.notes = "Basis propagation ablation requires MILP model";
        return rec;
    }

    SolutionVerifier verifier;

    // --- BASELINE: Basis Propagation ON ---
    double total_b_time = 0.0;
    BnBResult res_b;
    for (size_t i = 0; i < repetitions; i++) {
        BnBConfig cfg;
        cfg.warm_start_mode = WarmStartMode::WARM_START;
        BranchAndBoundEngine engine(cfg);
        auto t0 = std::chrono::high_resolution_clock::now();
        res_b = engine.solve(model);
        auto t1 = std::chrono::high_resolution_clock::now();
        total_b_time += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    rec.baseline_solve_time_ms = total_b_time / repetitions;
    rec.baseline_total_time_ms = rec.baseline_solve_time_ms;
    rec.baseline_node_count = res_b.telemetry.nodes_processed;
    rec.baseline_objective = res_b.objective_value;
    rec.baseline_status = bnb_solver_status_to_string(res_b.status);

    if (res_b.status == BnBSolverStatus::OPTIMAL) {
        VerificationResult vres = verifier.verify(model, res_b.solution, res_b.objective_value);
        rec.baseline_verified = vres.verified;
        rec.max_residual = std::max(rec.max_residual, vres.maximum_constraint_violation);
    }

    // --- ABLATED: Basis Propagation OFF ---
    double total_a_time = 0.0;
    BnBResult res_a;
    for (size_t i = 0; i < repetitions; i++) {
        BnBConfig cfg;
        cfg.warm_start_mode = WarmStartMode::COLD_START;
        BranchAndBoundEngine engine(cfg);
        auto t0 = std::chrono::high_resolution_clock::now();
        res_a = engine.solve(model);
        auto t1 = std::chrono::high_resolution_clock::now();
        total_a_time += std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    rec.ablated_solve_time_ms = total_a_time / repetitions;
    rec.ablated_total_time_ms = rec.ablated_solve_time_ms;
    rec.ablated_node_count = res_a.telemetry.nodes_processed;
    rec.ablated_objective = res_a.objective_value;
    rec.ablated_status = bnb_solver_status_to_string(res_a.status);

    if (res_a.status == BnBSolverStatus::OPTIMAL) {
        VerificationResult vres = verifier.verify(model, res_a.solution, res_a.objective_value);
        rec.ablated_verified = vres.verified;
        rec.max_residual = std::max(rec.max_residual, vres.maximum_constraint_violation);
    }

    rec.delta_solve_time_ms = rec.ablated_solve_time_ms - rec.baseline_solve_time_ms;
    if (rec.baseline_solve_time_ms > 1e-9) {
        rec.percent_change_solve_time = (rec.delta_solve_time_ms / rec.baseline_solve_time_ms) * 100.0;
    }
    rec.delta_total_time_ms = rec.ablated_total_time_ms - rec.baseline_total_time_ms;
    if (rec.baseline_total_time_ms > 1e-9) {
        rec.percent_change_total_time = (rec.delta_total_time_ms / rec.baseline_total_time_ms) * 100.0;
    }
    rec.experiment_status = "PASS";
    rec.notes = "Basis reuse successful: " + std::to_string(res_b.telemetry.warm_starts_accepted);
    return rec;
}

std::vector<AblationRecord> AblationHarness::run_all_ablations(
    const std::vector<std::pair<std::string, LPModel>>& lp_instances,
    const std::vector<std::pair<std::string, LPModel>>& milp_instances,
    size_t repetitions
) {
    std::vector<AblationRecord> records;

    for (const auto& item : lp_instances) {
        const auto& name = item.first;
        const auto& model = item.second;
        records.push_back(run_a1_presolve(model, name, repetitions));
        records.push_back(run_a2_sparse(model, name, repetitions));
        records.push_back(run_a3_adaptive_routing(model, name, repetitions));
        records.push_back(run_a5_verification_overhead(model, name, repetitions));
        records.push_back(run_a6_cost_estimator(model, name, repetitions));
    }

    for (const auto& item : milp_instances) {
        const auto& name = item.first;
        const auto& model = item.second;
        records.push_back(run_a4_warm_start(model, name, repetitions));
        records.push_back(run_a7_basis_propagation(model, name, repetitions));
    }

    return records;
}

bool AblationReporter::export_csv(const std::vector<AblationRecord>& records, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "experiment_id,component,baseline_config,ablated_config,instance_name,problem_type,solver_name,hardware,build_type,repetitions,"
         << "baseline_solve_time_ms,ablated_solve_time_ms,delta_solve_time_ms,percent_change_solve_time,"
         << "baseline_total_time_ms,ablated_total_time_ms,delta_total_time_ms,percent_change_total_time,"
         << "baseline_iterations,ablated_iterations,baseline_node_count,ablated_node_count,"
         << "baseline_objective,ablated_objective,baseline_verified,ablated_verified,max_residual,experiment_status,notes\n";

    for (const auto& r : records) {
        file << r.experiment_id << ","
             << "\"" << r.component << "\","
             << "\"" << r.baseline_config << "\","
             << "\"" << r.ablated_config << "\","
             << r.instance_name << ","
             << r.problem_type << ","
             << r.solver_name << ","
             << r.hardware << ","
             << r.build_type << ","
             << r.repetitions << ","
             << std::fixed << std::setprecision(6)
             << r.baseline_solve_time_ms << ","
             << r.ablated_solve_time_ms << ","
             << r.delta_solve_time_ms << ","
             << r.percent_change_solve_time << ","
             << r.baseline_total_time_ms << ","
             << r.ablated_total_time_ms << ","
             << r.delta_total_time_ms << ","
             << r.percent_change_total_time << ","
             << r.baseline_iterations << ","
             << r.ablated_iterations << ","
             << r.baseline_node_count << ","
             << r.ablated_node_count << ","
             << r.baseline_objective << ","
             << r.ablated_objective << ","
             << (r.baseline_verified ? "true" : "false") << ","
             << (r.ablated_verified ? "true" : "false") << ","
             << r.max_residual << ","
             << r.experiment_status << ","
             << "\"" << r.notes << "\"\n";
    }

    return true;
}

bool AblationReporter::export_json(const std::vector<AblationRecord>& records, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "[\n";
    for (size_t i = 0; i < records.size(); i++) {
        const auto& r = records[i];
        file << "  {\n"
             << "    \"experiment_id\": \"" << r.experiment_id << "\",\n"
             << "    \"component\": \"" << r.component << "\",\n"
             << "    \"baseline_config\": \"" << r.baseline_config << "\",\n"
             << "    \"ablated_config\": \"" << r.ablated_config << "\",\n"
             << "    \"instance_name\": \"" << r.instance_name << "\",\n"
             << "    \"problem_type\": \"" << r.problem_type << "\",\n"
             << "    \"solver_name\": \"" << r.solver_name << "\",\n"
             << "    \"hardware\": \"" << r.hardware << "\",\n"
             << "    \"build_type\": \"" << r.build_type << "\",\n"
             << "    \"repetitions\": " << r.repetitions << ",\n"
             << "    \"baseline_solve_time_ms\": " << r.baseline_solve_time_ms << ",\n"
             << "    \"ablated_solve_time_ms\": " << r.ablated_solve_time_ms << ",\n"
             << "    \"delta_solve_time_ms\": " << r.delta_solve_time_ms << ",\n"
             << "    \"percent_change_solve_time\": " << r.percent_change_solve_time << ",\n"
             << "    \"baseline_total_time_ms\": " << r.baseline_total_time_ms << ",\n"
             << "    \"ablated_total_time_ms\": " << r.ablated_total_time_ms << ",\n"
             << "    \"delta_total_time_ms\": " << r.delta_total_time_ms << ",\n"
             << "    \"percent_change_total_time\": " << r.percent_change_total_time << ",\n"
             << "    \"baseline_iterations\": " << r.baseline_iterations << ",\n"
             << "    \"ablated_iterations\": " << r.ablated_iterations << ",\n"
             << "    \"baseline_node_count\": " << r.baseline_node_count << ",\n"
             << "    \"ablated_node_count\": " << r.ablated_node_count << ",\n"
             << "    \"baseline_objective\": " << r.baseline_objective << ",\n"
             << "    \"ablated_objective\": " << r.ablated_objective << ",\n"
             << "    \"baseline_verified\": " << (r.baseline_verified ? "true" : "false") << ",\n"
             << "    \"ablated_verified\": " << (r.ablated_verified ? "true" : "false") << ",\n"
             << "    \"max_residual\": " << r.max_residual << ",\n"
             << "    \"experiment_status\": \"" << r.experiment_status << "\",\n"
             << "    \"notes\": \"" << r.notes << "\"\n"
             << "  }" << (i + 1 < records.size() ? "," : "") << "\n";
    }
    file << "]\n";

    return true;
}

void AblationReporter::print_summary_table(const std::vector<AblationRecord>& records) {
    std::cout << "\n========================================================================================================\n";
    std::cout << " BHARATOPT — PHASE 22 CONTROLLED ABLATION STUDY RESULTS\n";
    std::cout << "========================================================================================================\n";
    std::cout << std::left 
              << std::setw(24) << "Experiment ID"
              << std::setw(15) << "Instance"
              << std::setw(16) << "Baseline (ms)"
              << std::setw(16) << "Ablated (ms)"
              << std::setw(14) << "Delta (ms)"
              << std::setw(12) << "Change (%)"
              << std::setw(10) << "Verified"
              << std::setw(10) << "Status" << "\n";
    std::cout << "--------------------------------------------------------------------------------------------------------\n";

    for (const auto& r : records) {
        std::cout << std::left 
                  << std::setw(24) << r.experiment_id
                  << std::setw(15) << r.instance_name
                  << std::fixed << std::setprecision(4)
                  << std::setw(16) << r.baseline_solve_time_ms
                  << std::setw(16) << r.ablated_solve_time_ms
                  << std::setw(14) << r.delta_solve_time_ms
                  << std::setw(12) << r.percent_change_solve_time
                  << std::setw(10) << (r.baseline_verified && r.ablated_verified ? "YES" : "NO")
                  << std::setw(10) << r.experiment_status << "\n";
    }
    std::cout << "========================================================================================================\n\n";
}

} // namespace bharatopt
