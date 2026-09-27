#include "test_harness.hpp"
#include <bharatopt/dual_revised_simplex.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/educational_simplex.hpp>
#include <bharatopt/presolve.hpp>

using namespace bharatopt;

TEST_CASE(DualSimplex_01_IdentityDualFeasibleLP) {
    // Min 3x1 + 4x2
    // s.t. x1 + x2 >= 5  => -x1 - x2 <= -5 => -x1 - x2 + s1 = -5 (s1 = -5 < 0)
    //      x1 + 2x2 >= 6 => -x1 - 2x2 <= -6 => -x1 - 2x2 + s2 = -6 (s2 = -6 < 0)
    //      x1, x2 >= 0
    // Initial slack basis: s1 = -5, s2 = -6 (Primal Infeasible)
    // Reduced costs for nonbasic x1, x2: c1 = 3 >= 0, c2 = 4 >= 0 (Dual Feasible!)
    LPModel model("dual_01_basic");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 4.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 5.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 2.0}}, ConstraintSense::GREATER_EQUAL, 6.0);

    DualRevisedSimplexOptions opts;
    opts.solver_type = BasisSolverType::SPARSE_LU;
    DualRevisedSimplex solver(opts);
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 4.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[x2], 1.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 16.0, 1e-6);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(DualSimplex_02_SingleViolatedBasicVariable) {
    // Min 2x1 + 5x2
    // s.t. x1 + x2 >= 4  (s1 = -4 < 0)
    //      x1 <= 6       (s2 = 6 >= 0)
    //      x1, x2 >= 0
    LPModel model("single_violated");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 2.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 4.0);
    model.add_constraint("c2", {{x1, 1.0}}, ConstraintSense::LESS_EQUAL, 6.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 4.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 8.0, 1e-6);
}

TEST_CASE(DualSimplex_03_MultipleViolatedBasicVariables) {
    // Min 5x1 + 2x2 + 3x3
    // s.t. x1 + x2 + x3 >= 10 (s1 = -10)
    //      2x1 + x2 >= 8      (s2 = -8)
    //      x1, x2, x3 >= 0
    LPModel model("multi_violated");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 5.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 2.0);
    index_t x3 = model.add_variable("x3", 0.0, BHARATOPT_INFINITY, 3.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}, {x3, 1.0}}, ConstraintSense::GREATER_EQUAL, 10.0);
    model.add_constraint("c2", {{x1, 2.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 8.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x2], 10.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 20.0, 1e-6);
}

TEST_CASE(DualSimplex_04_OnePivotConvergence) {
    // Min 4x1 + 3x2
    // s.t. x1 + 2x2 >= 4 (s1 = -4)
    //      x1, x2 >= 0
    LPModel model("one_pivot");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 4.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 3.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 2.0}}, ConstraintSense::GREATER_EQUAL, 4.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_EQ(res.iterations, static_cast<size_t>(1));
    EXPECT_NEAR(res.primal_solution[x2], 2.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 6.0, 1e-6);
}

TEST_CASE(DualSimplex_05_MultiplePivotConvergence) {
    // Min 10x1 + 15x2 + 20x3
    // s.t. x1 + 2x2 + x3 >= 12
    //      2x1 + x2 + 3x3 >= 18
    //      x1, x2, x3 >= 0
    LPModel model("multi_pivot");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 10.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 15.0);
    index_t x3 = model.add_variable("x3", 0.0, BHARATOPT_INFINITY, 20.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 2.0}, {x3, 1.0}}, ConstraintSense::GREATER_EQUAL, 12.0);
    model.add_constraint("c2", {{x1, 2.0}, {x2, 1.0}, {x3, 3.0}}, ConstraintSense::GREATER_EQUAL, 18.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(DualSimplex_06_DegeneratePivot) {
    // LP with 0 RHS in constraint
    LPModel model("degenerate_dual");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 2.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 3.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 0.0);
    model.add_constraint("c2", {{x1, 2.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 4.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 4.0, 1e-6);
}

TEST_CASE(DualSimplex_07_TieInLeavingSelection) {
    // Equal violation basic variables: s1 = -6, s2 = -6
    LPModel model("tie_leaving");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 3.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 6.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 2.0}}, ConstraintSense::GREATER_EQUAL, 6.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 18.0, 1e-6);
}

TEST_CASE(DualSimplex_08_TieInEnteringSelection) {
    // Equal ratios in dual ratio test
    LPModel model("tie_entering");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 2.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 2.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 5.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 10.0, 1e-6);
}

TEST_CASE(DualSimplex_09_InfeasibleLP) {
    // Min x1 + x2 s.t. x1 + x2 <= -5 (impossible for x1, x2 >= 0)
    LPModel model("infeasible_dual");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 1.0);

    model.add_constraint("c1", {{x1, -1.0}, {x2, -1.0}}, ConstraintSense::GREATER_EQUAL, 5.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::INFEASIBLE);
}

TEST_CASE(DualSimplex_10_OptimalInitialBasis) {
    // Starting basis is already primal feasible (b >= 0 and c >= 0)
    LPModel model("optimal_initial");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 4.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_EQ(res.iterations, static_cast<size_t>(0));
    EXPECT_NEAR(res.objective_value, 0.0, 1e-6);
}

TEST_CASE(DualSimplex_11_ZeroObjectiveLP) {
    LPModel model("zero_obj");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 0.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 0.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 4.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 0.0, 1e-6);
}

TEST_CASE(DualSimplex_12_LowerBoundViolation) {
    // Min 2x1 + 3x2 s.t. x1 >= 3, x2 >= 2
    LPModel model("lower_bound_viol");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 3.0, BHARATOPT_INFINITY, 2.0);
    index_t x2 = model.add_variable("x2", 2.0, BHARATOPT_INFINITY, 3.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 6.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 4.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[x2], 2.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 14.0, 1e-6);
}

TEST_CASE(DualSimplex_13_UpperBoundViolation) {
    LPModel model("upper_bound_viol");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, 5.0, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 4.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 8.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 5.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[x2], 3.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 17.0, 1e-6);
}

TEST_CASE(DualSimplex_14_SmallCoefficients) {
    LPModel model("small_coeff");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1e-4);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 2e-4);

    model.add_constraint("c1", {{x1, 1e-4}, {x2, 1e-4}}, ConstraintSense::GREATER_EQUAL, 1.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 10000.0, 1e-2);
    EXPECT_NEAR(res.objective_value, 1.0, 1e-4);
}

TEST_CASE(DualSimplex_15_LargeCoefficientRange) {
    LPModel model("large_range");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 10000.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 10.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 10.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 10.0, 1e-6);
}

TEST_CASE(DualSimplex_16_SingularBasis) {
    // Tests solver handling when pivot tolerance causes factorisation failure
    LPModel model("singular_basis");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 1.0);

    model.add_constraint("c1", {{x1, 1e-6}, {x2, 1e-6}}, ConstraintSense::GREATER_EQUAL, 5.0);

    DualRevisedSimplexOptions opts;
    opts.pivot_tolerance = 0.5; // Force strict pivot tolerance failure
    DualRevisedSimplex solver(opts);
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::NUMERICAL_FAILURE);
}

TEST_CASE(DualSimplex_17_NumericalFailureCase) {
    LPModel model("num_failure");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);

    model.add_constraint("c1", {{x1, 0.0}}, ConstraintSense::GREATER_EQUAL, 5.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::INFEASIBLE);
}

TEST_CASE(DualSimplex_18_DenseVsSparseLUEquivalence) {
    LPModel model("equivalence_test");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 5.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 4.0);
    index_t x3 = model.add_variable("x3", 0.0, BHARATOPT_INFINITY, 6.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 2.0}, {x3, 1.0}}, ConstraintSense::GREATER_EQUAL, 15.0);
    model.add_constraint("c2", {{x1, 2.0}, {x2, 1.0}, {x3, 3.0}}, ConstraintSense::GREATER_EQUAL, 20.0);

    DualRevisedSimplexOptions opts_dense;
    opts_dense.solver_type = BasisSolverType::DENSE_LU;
    DualRevisedSimplex solver_dense(opts_dense);
    DualRevisedSimplexResult res_dense = solver_dense.solve(model);

    DualRevisedSimplexOptions opts_sparse;
    opts_sparse.solver_type = BasisSolverType::SPARSE_LU;
    DualRevisedSimplex solver_sparse(opts_sparse);
    DualRevisedSimplexResult res_sparse = solver_sparse.solve(model);

    EXPECT_EQ(res_dense.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_EQ(res_sparse.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res_dense.objective_value, res_sparse.objective_value, 1e-6);

    for (size_t j = 0; j < model.num_variables(); ++j) {
        EXPECT_NEAR(res_dense.primal_solution[j], res_sparse.primal_solution[j], 1e-6);
    }
}

TEST_CASE(DualSimplex_19_CrossSolverComparison) {
    // Cross validation: EducationalSimplex vs RevisedSimplex vs DualRevisedSimplex
    LPModel model("cross_solver_comp");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t y = model.add_variable("y", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    EducationalSimplex edu_solver;
    SimplexResult edu_res = edu_solver.solve(model);

    RevisedSimplex rev_solver;
    RevisedSimplexResult rev_res = rev_solver.solve(model);

    DualRevisedSimplex dual_solver;
    DualRevisedSimplexResult dual_res = dual_solver.solve(model);

    EXPECT_EQ(edu_res.status, SimplexStatus::OPTIMAL);
    EXPECT_EQ(rev_res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_EQ(dual_res.status, DualRevisedSimplexStatus::OPTIMAL);

    EXPECT_NEAR(edu_res.objective_value, 64.0 / 3.0, 1e-6);
    EXPECT_NEAR(rev_res.objective_value, 64.0 / 3.0, 1e-6);
    EXPECT_NEAR(dual_res.objective_value, 64.0 / 3.0, 1e-6);
}

TEST_CASE(DualSimplex_20_PresolveDualPostsolvePipeline) {
    LPModel model("presolve_dual_pipeline");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 2.0, 2.0, 4.0); // Fixed x1 = 2
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x3 = model.add_variable("x3", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}, {x3, 2.0}}, ConstraintSense::GREATER_EQUAL, 10.0);
    model.add_constraint("c2", {{x1, 2.0}, {x2, 2.0}, {x3, 1.0}}, ConstraintSense::GREATER_EQUAL, 12.0);

    PresolveEngine presolver;
    PresolveResult presolve_res = presolver.presolve(model);
    EXPECT_EQ(presolve_res.status, PresolveStatus::SUCCESS);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult dual_res = solver.solve(presolve_res.reduced_model);
    EXPECT_EQ(dual_res.status, DualRevisedSimplexStatus::OPTIMAL);

    std::vector<real_t> orig_x = presolve_res.postsolve.recover_solution(dual_res.primal_solution);
    EXPECT_NEAR(orig_x[x1], 2.0, 1e-6);
    EXPECT_TRUE(presolve_res.postsolve.verify_original_feasibility(orig_x));
}

TEST_CASE(DualSimplex_21_MandatoryHandDerivedLP) {
    // Mandatory Hand-Derived LP: Max 3x + 5y s.t. 2x + y <= 8, x + 2y <= 8
    // Optimum: x = 8/3, y = 8/3, obj = 64/3 (~21.333333)
    LPModel model("primary_hand_derived_dual");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t y = model.add_variable("y", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x], 8.0 / 3.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[y], 8.0 / 3.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 64.0 / 3.0, 1e-6);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(DualSimplex_22_UnsupportedInitialBasis) {
    // Minimization LP with negative cost in initial slack setup (not dual feasible at start)
    LPModel model("unsupported_initial");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, -5.0);

    model.add_constraint("c1", {{x1, 1.0}}, ConstraintSense::GREATER_EQUAL, 4.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::UNSUPPORTED_INITIAL_BASIS);
}

TEST_CASE(DualSimplex_23_IndependentVerifier) {
    LPModel model("independent_verifier");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 2.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 3.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 2.0}}, ConstraintSense::GREATER_EQUAL, 6.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
    real_t recomputed_obj = solver.recompute_original_objective(model, res.primal_solution);
    EXPECT_NEAR(recomputed_obj, res.objective_value, 1e-6);
}
