#include <bharatopt/mps_parser.hpp>
#include <bharatopt/presolve.hpp>
#include <bharatopt/branch_and_bound.hpp>
#include "test_harness.hpp"
#include <iostream>
#include <chrono>

namespace bharatopt {

TEST_CASE(ProfileMilpBnbProcurementComplexTest) {
    MpsParser parser;
    MpsParseResult parse_res = parser.parse_file("procurement_lp_complex.mps");

    std::cout << "\n=================================================================\n";
    std::cout << " MILP BRANCH & BOUND PROFILING REPORT: PROCUREMENT_LP_COMPLEX.MPS\n";
    std::cout << "=================================================================\n";
    std::cout << "File Parsed             : procurement_lp_complex.mps\n";
    std::cout << "Variables (N)           : " << parse_res.cols_parsed << "\n";
    std::cout << "Constraints (M)         : " << parse_res.rows_parsed << "\n";

    // 1. Solve Raw Model WITHOUT Presolve Big-M Tightening
    std::cout << "\n--- TEST 1: RAW MODEL MILP B&B (Big-M = 1000) ---\n";
    BnBConfig raw_cfg;
    raw_cfg.time_limit_ms = 5000.0;
    BranchAndBoundEngine raw_solver(raw_cfg);
    auto t0 = std::chrono::high_resolution_clock::now();
    BnBResult raw_res = raw_solver.solve(parse_res.model);
    auto t1 = std::chrono::high_resolution_clock::now();
    double raw_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "Raw MILP Status         : " << bnb_solver_status_to_string(raw_res.status) << "\n";
    std::cout << "Nodes Created           : " << raw_res.telemetry.nodes_created << "\n";
    std::cout << "Nodes Processed         : " << raw_res.telemetry.nodes_processed << "\n";
    std::cout << "Pruned by Bound         : " << raw_res.telemetry.nodes_pruned_bound << "\n";
    std::cout << "Pruned by Infeasibility : " << raw_res.telemetry.nodes_pruned_infeasibility << "\n";
    std::cout << "Integer Feasible Nodes  : " << raw_res.telemetry.integer_feasible_nodes << "\n";
    std::cout << "Solve Time (ms)         : " << raw_ms << " ms\n";
    std::cout << "Best Objective          : " << raw_res.objective_value << "\n";

    // 2. Solve WITH Presolve Big-M Tightening
    std::cout << "\n--- TEST 2: PRESOLVED MODEL MILP B&B (Tightened Big-M = 360-380) ---\n";
    PresolveEngine presolver;
    auto tp0 = std::chrono::high_resolution_clock::now();
    PresolveResult p_res = presolver.presolve(parse_res.model);
    auto tp1 = std::chrono::high_resolution_clock::now();
    double presolve_ms = std::chrono::duration<double, std::milli>(tp1 - tp0).count();

    std::cout << "Presolve Time (ms)      : " << presolve_ms << " ms\n";
    std::cout << "Transformations Count   : " << p_res.stats.transformations.size() << "\n";
    for (const auto& trans : p_res.stats.transformations) {
        if (trans.description.find("Big-M") != std::string::npos) {
            std::cout << "  - " << trans.description << "\n";
        }
    }

    // 3. Test WITH OPEN=1 Seeded Incumbent Heuristic
    std::cout << "\n--- TEST 3: MILP B&B WITH SEEDED OPEN=1 INCUMBENT HEURISTIC ---\n";
    LPModel open1_fixed_model = p_res.reduced_model;
    for (size_t j = 0; j < open1_fixed_model.num_variables(); ++j) {
        const auto& v = open1_fixed_model.get_variable(static_cast<index_t>(j));
        if (v.type == VariableType::BINARY || v.type == VariableType::INTEGER) {
            auto& mod_v = open1_fixed_model.get_variable(static_cast<index_t>(j));
            mod_v.lower_bound = 1.0;
            mod_v.upper_bound = 1.0;
        }
    }

    DualRevisedSimplex seed_solver;
    DualRevisedSimplexResult seed_lp_res = seed_solver.solve(open1_fixed_model);

    std::cout << "Seeded Sub-LP Status    : " << (seed_lp_res.status == DualRevisedSimplexStatus::OPTIMAL ? "OPTIMAL" : "OTHER") << "\n";
    if (seed_lp_res.status == DualRevisedSimplexStatus::OPTIMAL) {
        std::vector<real_t> full_seed_sol = p_res.postsolve.recover_solution(seed_lp_res.primal_solution);
        real_t seed_orig_obj = p_res.postsolve.compute_original_objective(full_seed_sol);
        std::cout << "Seeded Integer Objective: " << seed_orig_obj << "\n";
    }

    BnBConfig p_cfg;
    p_cfg.time_limit_ms = 5000.0; // Identical 5s budget
    BranchAndBoundEngine p_solver(p_cfg);
    t0 = std::chrono::high_resolution_clock::now();
    BnBResult p_res_solve = p_solver.solve(p_res.reduced_model);
    t1 = std::chrono::high_resolution_clock::now();
    double p_solve_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::vector<real_t> full_sol = p_res.postsolve.recover_solution(p_res_solve.solution);
    real_t orig_obj = p_res.postsolve.compute_original_objective(full_sol);

    std::cout << "Presolved MILP Status   : " << bnb_solver_status_to_string(p_res_solve.status) << "\n";
    std::cout << "Nodes Created           : " << p_res_solve.telemetry.nodes_created << "\n";
    std::cout << "Nodes Processed         : " << p_res_solve.telemetry.nodes_processed << "\n";
    std::cout << "Pruned by Bound         : " << p_res_solve.telemetry.nodes_pruned_bound << "\n";
    std::cout << "Pruned by Infeasibility : " << p_res_solve.telemetry.nodes_pruned_infeasibility << "\n";
    std::cout << "Integer Feasible Nodes  : " << p_res_solve.telemetry.integer_feasible_nodes << "\n";
    std::cout << "Presolved Solve Time    : " << p_solve_ms << " ms\n";
    std::cout << "Presolved Objective     : " << p_res_solve.objective_value << "\n";
    std::cout << "Original Model Objective: " << orig_obj << "\n";
    std::cout << "=================================================================\n\n";

    EXPECT_EQ(parse_res.status, MpsParseStatus::SUCCESS);
}

} // namespace bharatopt
