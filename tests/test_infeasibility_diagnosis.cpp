#include <bharatopt/model_validator.hpp>
#include <bharatopt/solution_verifier.hpp>
#include <bharatopt/mps_parser.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/presolve.hpp>
#include "test_harness.hpp"
#include <iostream>

namespace bharatopt {

TEST_CASE(InfeasibilityDiagnosisTest) {
    // 1. Test Infeasibility Diagnosis on share2b original infeasible model
    MpsParser parser;
    MpsParseResult parse_share2b = parser.parse_file("benchmarks/netlib/share2b.mps");
    
    // Create an artificial infeasible model if share2b was modified: C03 (X01>=4), C04 (X02=3), C01 (2X01+X02<=8)
    LPModel infeas_model("infeasible_share2b");
    index_t x1 = infeas_model.add_variable("X01", 0.0, 10.0, 3.0);
    index_t x2 = infeas_model.add_variable("X02", 0.0, 10.0, 5.0);

    // C01: 2*X01 + X02 <= 8.0
    infeas_model.add_constraint("C01", {{x1, 2.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    // C03: X01 >= 4.0
    infeas_model.add_constraint("C03", {{x1, 1.0}}, ConstraintSense::GREATER_EQUAL, 4.0);
    // C04: X02 = 3.0
    infeas_model.add_constraint("C04", {{x2, 1.0}}, ConstraintSense::EQUAL, 3.0);

    InfeasibilityDiagnosis diag1 = InfeasibilityAnalyzer::analyze(infeas_model);
    std::cout << "\n=== INFEASIBILITY DIAGNOSIS TEST 1 ===\n";
    std::cout << "Is Infeasible: " << (diag1.is_infeasible ? "YES" : "NO") << "\n";
    std::cout << "Cert String  : " << diag1.to_string() << "\n";

    EXPECT_TRUE(diag1.is_infeasible);
    EXPECT_FALSE(diag1.conflicting_rows.empty());

    // 2. Test Procurement LP with RANGES infeasibility
    LPModel proc_model("infeasible_procurement");
    std::vector<index_t> s1_vars, s2_vars, s3_vars, s4_vars;
    std::vector<std::pair<index_t, real_t>> all_terms;

    for (int i = 1; i <= 5; ++i) {
        for (int j = 1; j <= 4; ++j) {
            index_t v = proc_model.add_variable("X" + std::to_string(i) + "_" + std::to_string(j), 0.0, 100.0, 10.0);
            all_terms.push_back({v, 1.0});
            if (j == 1) s1_vars.push_back(v);
            else if (j == 2) s2_vars.push_back(v);
            else if (j == 3) s3_vars.push_back(v);
            else if (j == 4) s4_vars.push_back(v);
        }
    }

    // SLOT1CAP: ranged [350, 400]
    std::vector<std::pair<index_t, real_t>> t1, t2, t3, t4;
    for (auto v : s1_vars) t1.push_back({v, 1.0});
    for (auto v : s2_vars) t2.push_back({v, 1.0});
    for (auto v : s3_vars) t3.push_back({v, 1.0});
    for (auto v : s4_vars) t4.push_back({v, 1.0});

    proc_model.add_constraint("SLOT1CAP", t1, ConstraintSense::RANGED, 350.0, 400.0);
    proc_model.add_constraint("SLOT2CAP", t2, ConstraintSense::RANGED, 310.0, 350.0);
    proc_model.add_constraint("SLOT3CAP", t3, ConstraintSense::RANGED, 270.0, 300.0);
    proc_model.add_constraint("SLOT4CAP", t4, ConstraintSense::RANGED, 220.0, 250.0);
    proc_model.add_constraint("TOTFARM", all_terms, ConstraintSense::EQUAL, 1000.0);

    InfeasibilityDiagnosis diag2 = InfeasibilityAnalyzer::analyze(proc_model);
    std::cout << "\n=== INFEASIBILITY DIAGNOSIS TEST 2 (PROCUREMENT) ===\n";
    std::cout << "Is Infeasible: " << (diag2.is_infeasible ? "YES" : "NO") << "\n";
    std::cout << "Cert String  : " << diag2.to_string() << "\n";

    EXPECT_TRUE(diag2.is_infeasible);
    EXPECT_TRUE(diag2.to_string().find("1150") != std::string::npos || diag2.to_string().find("forced min") != std::string::npos);

    // 3. Regression test: Genuine Infeasible MPS (procurement_lp_test.mps)
    MpsParseResult proc_orig = parser.parse_file("procurement_lp_test.mps");
    EXPECT_EQ(static_cast<int>(proc_orig.status), static_cast<int>(MpsParseStatus::SUCCESS));
    InfeasibilityDiagnosis diag_orig = InfeasibilityAnalyzer::analyze(proc_orig.model);
    std::cout << "\n=== REGRESSION TEST: procurement_lp_test.mps (ORIGINAL INFEASIBLE) ===\n";
    std::cout << "Is Infeasible: " << (diag_orig.is_infeasible ? "YES" : "NO") << "\n";
    std::cout << "Conflicting Rows: ";
    for (size_t r = 0; r < diag_orig.conflicting_rows.size(); ++r) {
        std::cout << diag_orig.conflicting_rows[r] << (r + 1 < diag_orig.conflicting_rows.size() ? " ∩ " : "");
    }
    std::cout << "\nCert String  : " << diag_orig.to_string() << "\n";
    EXPECT_TRUE(diag_orig.is_infeasible);

    RevisedSimplex proc_solver;
    RevisedSimplexResult proc_sol_res = proc_solver.solve(proc_orig.model);
    EXPECT_EQ(proc_sol_res.status, RevisedSimplexStatus::INFEASIBLE);

    // 4. Regression test: Feasible Fixed MPS (procurement_lp_test_fixed.mps) (Expected Obj: 12880)
    MpsParseResult proc_fixed = parser.parse_file("procurement_lp_test_fixed.mps");
    EXPECT_EQ(static_cast<int>(proc_fixed.status), static_cast<int>(MpsParseStatus::SUCCESS));
    InfeasibilityDiagnosis diag_fixed = InfeasibilityAnalyzer::analyze(proc_fixed.model);
    std::cout << "\n=== REGRESSION TEST: procurement_lp_test_fixed.mps (FIXED FEASIBLE) ===\n";
    std::cout << "Is Infeasible: " << (diag_fixed.is_infeasible ? "YES" : "NO") << "\n";
    EXPECT_FALSE(diag_fixed.is_infeasible);

    RevisedSimplexResult proc_fixed_res = proc_solver.solve(proc_fixed.model);
    EXPECT_EQ(proc_fixed_res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(proc_fixed_res.objective_value, 12880.0, 1e-4);

    // 5. Regression test: example3.mps (min X; X>=5; X<=10; correct optimum X=5, obj=5)
    MpsParseResult ex3 = parser.parse_file("example3.mps");
    EXPECT_EQ(static_cast<int>(ex3.status), static_cast<int>(MpsParseStatus::SUCCESS));
    
    SolutionVerifier verifier;
    // Candidate x = 1.5 MUST fail on row MIN5
    VerificationResult v_fail = verifier.verify(ex3.model, {1.5});
    std::cout << "\n=== REGRESSION TEST: example3.mps with x=1.5 (MUST FAIL) ===\n";
    std::cout << "Verifier Status: " << (v_fail.verified ? "PASS" : "FAIL (CORRECT)") << "\n";
    std::cout << "Message: " << v_fail.status_message << "\n";
    EXPECT_FALSE(v_fail.verified);
    EXPECT_TRUE(v_fail.violated_constraint_count > 0);

    // Candidate x = 5.0 MUST pass and recompute objective = 5.0
    VerificationResult v_pass = verifier.verify(ex3.model, {5.0}, 5.0);
    std::cout << "\n=== REGRESSION TEST: example3.mps with x=5.0 (MUST PASS) ===\n";
    std::cout << "Verifier Status: " << (v_pass.verified ? "PASS" : "FAIL") << "\n";
    std::cout << "Recomputed Obj : " << v_pass.objective_recomputed << "\n";
    EXPECT_TRUE(v_pass.verified);
    EXPECT_NEAR(v_pass.objective_recomputed, 5.0, 1e-6);

    // Full solver end-to-end check on example3.mps with Presolve
    PresolveEngine p_engine;
    PresolveResult ex3_presolve = p_engine.presolve(ex3.model);
    RevisedSimplexResult ex3_sol = proc_solver.solve(ex3_presolve.reduced_model);
    std::vector<real_t> ex3_full_x = ex3_presolve.postsolve.recover_solution(ex3_sol.primal_solution);
    real_t ex3_full_obj = ex3_presolve.postsolve.compute_original_objective(ex3_full_x);

    EXPECT_NEAR(ex3_full_x[0], 5.0, 1e-6);
    EXPECT_NEAR(ex3_full_obj, 5.0, 1e-6);
    VerificationResult ex3_endtoend_v = verifier.verify(ex3.model, ex3_full_x, ex3_full_obj);
    EXPECT_TRUE(ex3_endtoend_v.verified);
}

} // namespace bharatopt
