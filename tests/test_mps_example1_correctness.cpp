#include <bharatopt/mps_parser.hpp>
#include <bharatopt/model_validator.hpp>
#include <bharatopt/presolve.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/dual_revised_simplex.hpp>
#include <bharatopt/educational_simplex.hpp>
#include <bharatopt/solution_verifier.hpp>
#include "test_harness.hpp"
#include <iostream>

namespace bharatopt {

TEST_CASE(Phase26_Example1MpsCorrectness) {
    MpsParser parser;
    MpsParseResult parse_res = parser.parse_file("example1.mps");
    
    std::cout << "\n=== EXAMPLE1 PARSER DIAGNOSTIC ===\n";
    std::cout << "Parse Status : " << (parse_res.status == MpsParseStatus::SUCCESS ? "SUCCESS" : "FAIL") << "\n";
    std::cout << "Problem Name : " << parse_res.problem_name << "\n";
    std::cout << "Obj Sense    : " << (parse_res.model.sense() == ObjectiveSense::MAXIMIZE ? "MAXIMIZE" : "MINIMIZE") << "\n";
    std::cout << "Variables (N): " << parse_res.cols_parsed << "\n";
    std::cout << "Constraints M: " << parse_res.rows_parsed << "\n";
    std::cout << "Matrix NNZ   : " << parse_res.nonzeros_parsed << "\n";

    for (size_t j = 0; j < parse_res.model.num_variables(); ++j) {
        const auto& v = parse_res.model.get_variable(static_cast<index_t>(j));
        std::cout << "  Var[" << j << "] " << v.name << " : lb=" << v.lower_bound << ", ub=" << v.upper_bound << ", c=" << v.obj_coeff << "\n";
    }

    for (size_t i = 0; i < parse_res.model.num_constraints(); ++i) {
        const auto& c = parse_res.model.get_constraint(static_cast<index_t>(i));
        std::cout << "  Cons[" << i << "] " << c.name << " : rhs=" << c.rhs << " terms=[";
        for (const auto& t : c.terms) {
            std::cout << "(" << parse_res.model.get_variable(t.first).name << "," << t.second << ") ";
        }
        std::cout << "]\n";
    }

    EXPECT_EQ(parse_res.status, MpsParseStatus::SUCCESS);
    EXPECT_EQ(parse_res.model.sense(), ObjectiveSense::MAXIMIZE);
    EXPECT_EQ(parse_res.cols_parsed, 2);
    EXPECT_EQ(parse_res.rows_parsed, 3);
    EXPECT_EQ(parse_res.nonzeros_parsed, 4);

    // Test Manual Solution Verification
    std::vector<real_t> manual_sol = {2.0, 6.0}; // x1=2, x2=6
    SolutionVerifier verifier;
    VerificationResult v_res = verifier.verify(parse_res.model, manual_sol, 36.0);
    std::cout << "\n=== MANUAL SOLUTION VERIFIER ===\n";
    std::cout << "Verified     : " << (v_res.verified ? "PASS" : "FAIL") << "\n";
    std::cout << "Recomputed Z : " << v_res.objective_recomputed << "\n";
    std::cout << "Max Cons Viol: " << v_res.maximum_constraint_violation << "\n";
    EXPECT_TRUE(v_res.verified);
    EXPECT_NEAR(v_res.objective_recomputed, 36.0, 1e-6);

    // Test Presolve Engine
    PresolveEngine presolver;
    PresolveResult p_res = presolver.presolve(parse_res.model);
    std::cout << "\n=== PRESOLVE DIAGNOSTIC ===\n";
    std::cout << p_res.to_string();

    // Test Revised Simplex on Original & Presolved Model
    RevisedSimplex solver;
    RevisedSimplexResult solve_res_orig = solver.solve(parse_res.model);
    std::cout << "\n=== REVISED SIMPLEX (ORIGINAL MODEL) ===\n";
    std::cout << "Status   : " << static_cast<int>(solve_res_orig.status) << "\n";
    std::cout << "Objective: " << solve_res_orig.objective_value << "\n";
    for (size_t j = 0; j < solve_res_orig.primal_solution.size(); ++j) {
        std::cout << "  x[" << j << "] = " << solve_res_orig.primal_solution[j] << "\n";
    }

    RevisedSimplexResult solve_res_red = solver.solve(p_res.reduced_model);
    std::cout << "\n=== REVISED SIMPLEX (PRESOLVED MODEL) ===\n";
    std::cout << "Status   : " << static_cast<int>(solve_res_red.status) << "\n";
    std::cout << "Objective: " << solve_res_red.objective_value << "\n";
    for (size_t j = 0; j < solve_res_red.primal_solution.size(); ++j) {
        std::cout << "  x[" << j << "] = " << solve_res_red.primal_solution[j] << "\n";
    }
}

} // namespace bharatopt
