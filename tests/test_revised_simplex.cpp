#include "test_harness.hpp"
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/educational_simplex.hpp>
#include <bharatopt/presolve.hpp>

using namespace bharatopt;

TEST_CASE(RevisedSimplex_01_Basic2VariableLP) {
    // Primary Hand-Derived Benchmark LP:
    // Max 3x + 5y s.t. 2x + y <= 8, x + 2y <= 8, x, y >= 0
    // Expected solution: x = 8/3, y = 8/3, obj = 64/3 (~21.33333333)
    LPModel model("primary_hand_derived");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t y = model.add_variable("y", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x], 8.0 / 3.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[y], 8.0 / 3.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 64.0 / 3.0, 1e-6);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(RevisedSimplex_02_MultiplePivotLP) {
    LPModel model("multi_pivot");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 10.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 6.0);
    index_t x3 = model.add_variable("x3", 0.0, BHARATOPT_INFINITY, 4.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 100.0);
    model.add_constraint("c2", {{x1, 10.0}, {x2, 4.0}, {x3, 5.0}}, ConstraintSense::LESS_EQUAL, 600.0);
    model.add_constraint("c3", {{x1, 2.0}, {x2, 2.0}, {x3, 6.0}}, ConstraintSense::LESS_EQUAL, 300.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_TRUE(res.iterations >= 2);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(RevisedSimplex_03_OneVariableLP) {
    LPModel model("one_var");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 4.0);
    model.add_constraint("c1", {{x1, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 5.0, 1e-7);
    EXPECT_NEAR(res.objective_value, 20.0, 1e-7);
}

TEST_CASE(RevisedSimplex_04_OneConstraintLP) {
    LPModel model("one_constraint");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 2.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 3.0);
    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 10.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 0.0, 1e-7);
    EXPECT_NEAR(res.primal_solution[x2], 10.0, 1e-7);
    EXPECT_NEAR(res.objective_value, 30.0, 1e-7);
}

TEST_CASE(RevisedSimplex_05_ThreeVariableLP) {
    LPModel model("three_variable");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 2.0);
    index_t x3 = model.add_variable("x3", 0.0, BHARATOPT_INFINITY, 4.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}, {x3, 2.0}}, ConstraintSense::LESS_EQUAL, 4.0);
    model.add_constraint("c2", {{x1, 2.0}, {x2, 0.5}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);
    model.add_constraint("c3", {{x1, 3.0}, {x2, 2.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 7.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
    EXPECT_NEAR(res.objective_value, 10.0, 1e-6);
}

TEST_CASE(RevisedSimplex_06_RedundantConstraintLP) {
    LPModel model("redundant");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t y = model.add_variable("y", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c_redundant", {{x, 1.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 20.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 64.0 / 3.0, 1e-6);
}

TEST_CASE(RevisedSimplex_07_DegenerateLP) {
    LPModel model("degenerate");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 2.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 0.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, -1.0}}, ConstraintSense::LESS_EQUAL, 0.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 0.0, 1e-7);
}

TEST_CASE(RevisedSimplex_08_ZeroRHS) {
    LPModel model("zero_rhs");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 5.0);
    model.add_constraint("c1", {{x1, 1.0}}, ConstraintSense::LESS_EQUAL, 0.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 0.0, 1e-7);
}

TEST_CASE(RevisedSimplex_09_MultipleOptimalSolutions) {
    // Max x1 + x2 s.t. x1 + x2 <= 5 => Multiple optima along segment (0,5)-(5,0)
    LPModel model("multi_optima");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 1.0);
    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 5.0, 1e-7);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(RevisedSimplex_10_UnboundedLP) {
    LPModel model("unbounded");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 1.0);
    model.add_constraint("c1", {{x1, 1.0}, {x2, -1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::UNBOUNDED);
}

TEST_CASE(RevisedSimplex_11_InfeasibleLP) {
    LPModel model("infeasible");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 1.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 2.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 5.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::INFEASIBLE);
}

TEST_CASE(RevisedSimplex_12_LowerBoundVariables) {
    LPModel model("lower_bounds");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 2.0, BHARATOPT_INFINITY, 3.0); // Shifted x1 = x1' + 2
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 1.0);
    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 10.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 10.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 30.0, 1e-6);
}

TEST_CASE(RevisedSimplex_13_FreeVariables) {
    LPModel model("free_variables");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", -BHARATOPT_INFINITY, BHARATOPT_INFINITY, 2.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 3.0);
    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::EQUAL, 5.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 5.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 10.0, 1e-6);
}

TEST_CASE(RevisedSimplex_14_EqualityConstraints) {
    LPModel model("equality_constraint");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 4.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 2.0);
    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::EQUAL, 6.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 6.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 24.0, 1e-6);
}

TEST_CASE(RevisedSimplex_15_GreaterThanConstraints) {
    LPModel model("greater_than");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 2.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 1.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 4.0);
    model.add_constraint("c2", {{x1, 1.0}}, ConstraintSense::LESS_EQUAL, 3.0);
    model.add_constraint("c3", {{x2, 1.0}}, ConstraintSense::LESS_EQUAL, 3.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 3.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[x2], 3.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 9.0, 1e-6);
}

TEST_CASE(RevisedSimplex_16_LessThanConstraints) {
    LPModel model("less_than");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);
    model.add_constraint("c1", {{x1, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 8.0, 1e-7);
}

TEST_CASE(RevisedSimplex_17_ArtificialVariablePhaseI) {
    LPModel model("artificial_phase1");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);
    model.add_constraint("c1", {{x1, 1.0}}, ConstraintSense::EQUAL, 4.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 4.0, 1e-7);
}

TEST_CASE(RevisedSimplex_18_SmallCoefficients) {
    LPModel model("small_coeff");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1e-5);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 2e-5);
    model.add_constraint("c1", {{x1, 1e-5}, {x2, 1e-5}}, ConstraintSense::LESS_EQUAL, 1.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x2], 1e5, 1e-1);
}

TEST_CASE(RevisedSimplex_19_LargeCoefficientRanges) {
    LPModel model("large_coeff_range");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 1000.0);
    model.add_constraint("c1", {{x1, 1.0}, {x2, 1000.0}}, ConstraintSense::LESS_EQUAL, 10000.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 10000.0, 1e-6);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(RevisedSimplex_20_NearlyDependentConstraints) {
    LPModel model("nearly_dependent");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 2.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 1.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 10.0);
    model.add_constraint("c2", {{x1, 1.0000001}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 10.0000001);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(RevisedSimplex_21_IterationLimitTest) {
    LPModel model("iter_limit");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 10.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 6.0);
    index_t x3 = model.add_variable("x3", 0.0, BHARATOPT_INFINITY, 4.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 100.0);
    model.add_constraint("c2", {{x1, 10.0}, {x2, 4.0}, {x3, 5.0}}, ConstraintSense::LESS_EQUAL, 600.0);

    RevisedSimplexOptions opts;
    opts.max_iterations = 1;
    RevisedSimplex solver(opts);

    RevisedSimplexResult res = solver.solve(model);
    EXPECT_EQ(res.status, RevisedSimplexStatus::ITERATION_LIMIT);
}

TEST_CASE(RevisedSimplex_22_NumericalFailureTest) {
    // Tests solver detection of singular basis factorization
    StandardFormLP lp;
    lp.num_rows = 2;
    lp.num_cols = 2;
    lp.A = {{0.0, 0.0}, {0.0, 0.0}}; // Singular basis matrix B
    Basis basis(2, 2);
    basis.set_initial_basis({0, 1});

    DenseBasisSolver solver;
    EXPECT_FALSE(solver.factorize(lp, basis));
}

TEST_CASE(RevisedSimplex_23_BasisInvariantTest) {
    Basis basis(3, 7);
    EXPECT_TRUE(basis.set_initial_basis({0, 1, 2}));
    EXPECT_TRUE(basis.check_invariants());

    // Pivot position 1 with variable 4
    EXPECT_TRUE(basis.update_basis(1, 4));
    EXPECT_TRUE(basis.check_invariants());
    EXPECT_EQ(basis.basic_vars[1], 4);
    EXPECT_EQ(basis.var_to_basic_pos[4], 1);
    EXPECT_EQ(basis.var_to_basic_pos[1], -1);
}

TEST_CASE(RevisedSimplex_24_EnteringVariableTest) {
    Basis basis(2, 4);
    basis.set_initial_basis({2, 3});

    StandardFormLP lp;
    lp.num_rows = 2;
    lp.num_cols = 4;
    lp.is_artificial.assign(4, false);

    // Reduced costs for non-basic cols 0 and 1
    std::vector<real_t> r = {-2.5, -5.0, 0.0, 0.0};

    RevisedSimplexOptions opts_bland;
    opts_bland.entering_rule = EnteringRule::BLANDS_RULE;
    RevisedSimplex solver_bland(opts_bland);
    EXPECT_EQ(solver_bland.select_entering_variable(basis, r, lp), 0); // Bland picks smallest index 0

    RevisedSimplexOptions opts_dantzig;
    opts_dantzig.entering_rule = EnteringRule::MOST_NEGATIVE;
    RevisedSimplex solver_dantzig(opts_dantzig);
    EXPECT_EQ(solver_dantzig.select_entering_variable(basis, r, lp), 1); // Dantzig picks most negative -5.0 (index 1)
}

TEST_CASE(RevisedSimplex_25_LeavingVariableTest) {
    Basis basis(2, 4);
    basis.set_initial_basis({2, 3});

    std::vector<real_t> x_B = {8.0, 8.0};
    std::vector<real_t> d_B = {1.0, 2.0}; // Row 0 ratio = 8/1 = 8, Row 1 ratio = 8/2 = 4

    RevisedSimplex solver;
    real_t min_ratio = 0.0;
    index_t leave_pos = solver.ratio_test(x_B, d_B, basis, min_ratio);

    EXPECT_EQ(leave_pos, 1); // Row 1 leaves (smaller ratio 4.0)
    EXPECT_NEAR(min_ratio, 4.0, 1e-7);
}

TEST_CASE(RevisedSimplex_26_RatioTestEdgeCases) {
    Basis basis(2, 4);
    basis.set_initial_basis({2, 3});

    std::vector<real_t> x_B = {5.0, 10.0};
    std::vector<real_t> d_B_unbounded = {-1.0, 0.0}; // Non-positive direction => Unbounded

    RevisedSimplex solver;
    real_t min_ratio = 0.0;
    index_t leave_pos = solver.ratio_test(x_B, d_B_unbounded, basis, min_ratio);
    EXPECT_EQ(leave_pos, -1);
}

TEST_CASE(RevisedSimplex_27_ObjectiveRecomputation) {
    LPModel model("obj_recomp");
    model.set_obj_offset(50.0);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 4.0);
    (void)x1;
    (void)x2;

    std::vector<real_t> solution = {2.0, 3.0};
    RevisedSimplex solver;
    real_t obj = solver.recompute_original_objective(model, solution);

    // 50 + 3*2 + 4*3 = 68.0
    EXPECT_NEAR(obj, 68.0, 1e-12);
}

TEST_CASE(RevisedSimplex_28_FeasibilityVerification) {
    LPModel model("feas_verification");
    index_t x1 = model.add_variable("x1", 0.0, 10.0);
    index_t x2 = model.add_variable("x2", 0.0, 10.0);
    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 12.0);

    RevisedSimplex solver;
    std::vector<real_t> valid_sol = {5.0, 5.0};
    std::vector<real_t> invalid_sol = {7.0, 7.0}; // Violates constraint (14 > 12)

    EXPECT_TRUE(solver.verify_solution_feasibility(model, valid_sol));
    EXPECT_FALSE(solver.verify_solution_feasibility(model, invalid_sol));
}

TEST_CASE(RevisedSimplex_29_PresolveRevisedPostsolvePipeline) {
    // Integrated Presolve -> Revised Simplex -> Postsolve Pipeline Test
    LPModel model("pipeline_test");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 4.0, 4.0, 2.0); // Fixed x1 = 4
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x3 = model.add_variable("x3", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x1, 2.0}, {x2, 1.0}, {x3, 2.0}}, ConstraintSense::LESS_EQUAL, 20.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 2.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 16.0);

    // Step 1: Run Presolve
    PresolveEngine presolver;
    PresolveResult presolve_res = presolver.presolve(model);
    EXPECT_EQ(presolve_res.status, PresolveStatus::SUCCESS);

    // Step 2: Solve Reduced Model with Revised Simplex
    RevisedSimplex solver;
    RevisedSimplexResult rev_res = solver.solve(presolve_res.reduced_model);
    EXPECT_EQ(rev_res.status, RevisedSimplexStatus::OPTIMAL);

    // Step 3: Postsolve mapping back to Original Variable Space
    std::vector<real_t> orig_x = presolve_res.postsolve.recover_solution(rev_res.primal_solution);

    // Step 4: Verify Original Model Feasibility & Objective
    EXPECT_NEAR(orig_x[x1], 4.0, 1e-7);
    EXPECT_TRUE(presolve_res.postsolve.verify_original_feasibility(orig_x));
    real_t orig_obj = presolve_res.postsolve.compute_original_objective(orig_x);
    EXPECT_NEAR(orig_obj, rev_res.objective_value, 1e-5);
}

TEST_CASE(RevisedSimplex_30_EducationalVsRevisedComparison) {
    // Controlled comparison between EducationalSimplex and RevisedSimplex
    LPModel model("comparison_model");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x1, 2.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    EducationalSimplex edu_solver;
    SimplexResult edu_res = edu_solver.solve(model);

    RevisedSimplex rev_solver;
    RevisedSimplexResult rev_res = rev_solver.solve(model);

    // Compare Status
    EXPECT_EQ(edu_res.status, SimplexStatus::OPTIMAL);
    EXPECT_EQ(rev_res.status, RevisedSimplexStatus::OPTIMAL);

    // Compare Objective Value
    EXPECT_NEAR(edu_res.objective_value, rev_res.objective_value, 1e-6);

    // Compare Primal Solution Vectors
    EXPECT_NEAR(edu_res.primal_solution[x1], rev_res.primal_solution[x1], 1e-6);
    EXPECT_NEAR(edu_res.primal_solution[x2], rev_res.primal_solution[x2], 1e-6);

    // Verify Independent Feasibility
    EXPECT_TRUE(rev_solver.verify_solution_feasibility(model, rev_res.primal_solution));
}
