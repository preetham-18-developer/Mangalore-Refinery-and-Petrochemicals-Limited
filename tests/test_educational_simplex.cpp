#include "test_harness.hpp"
#include <bharatopt/educational_simplex.hpp>
#include <bharatopt/presolve.hpp>

using namespace bharatopt;

TEST_CASE(Simplex_01_PivotLevelTest) {
    // Hand-calculated 2x3 tableau test:
    // Max 3x1 + 5x2
    // 2x1 + x2 + s1 = 8
    // x1 + 2x2 + s2 = 8
    // Obj row: -3x1 - 5x2 = 0
    Tableau tableau;
    tableau.num_rows = 2;
    tableau.num_cols = 4;
    tableau.col_names = {"x1", "x2", "s1", "s2"};
    tableau.basis = {2, 3}; // s1 (idx 2), s2 (idx 3)
    tableau.matrix = {
        {2.0, 1.0, 1.0, 0.0, 8.0}, // Row 0
        {1.0, 2.0, 0.0, 1.0, 8.0}, // Row 1
        {-3.0, -5.0, 0.0, 0.0, 0.0} // Obj row
    };

    EducationalSimplex solver;

    // Pivot at row 1, col 1 (entering x2, leaving s2)
    // Pivot element = 2.0
    bool success = solver.pivot(tableau, 1, 1);
    EXPECT_TRUE(success);

    // Expected Tableau matrix after 1 pivot:
    // Row 1 (divided by 2.0): 0.5, 1.0, 0.0, 0.5, 4.0
    // Row 0 (R0 - 1.0 * R1'): (2 - 0.5)=1.5, (1-1)=0.0, 1.0, (0-0.5)=-0.5, (8-4)=4.0
    // Obj row (Obj - (-5.0)*R1'): (-3 + 2.5)=-0.5, (-5 + 5)=0.0, 0.0, (0 + 2.5)=2.5, (0 + 20)=20.0
    EXPECT_NEAR(tableau.matrix[1][0], 0.5, 1e-12);
    EXPECT_NEAR(tableau.matrix[1][1], 1.0, 1e-12);
    EXPECT_NEAR(tableau.matrix[1][2], 0.0, 1e-12);
    EXPECT_NEAR(tableau.matrix[1][3], 0.5, 1e-12);
    EXPECT_NEAR(tableau.matrix[1][4], 4.0, 1e-12);

    EXPECT_NEAR(tableau.matrix[0][0], 1.5, 1e-12);
    EXPECT_NEAR(tableau.matrix[0][1], 0.0, 1e-12);
    EXPECT_NEAR(tableau.matrix[0][2], 1.0, 1e-12);
    EXPECT_NEAR(tableau.matrix[0][3], -0.5, 1e-12);
    EXPECT_NEAR(tableau.matrix[0][4], 4.0, 1e-12);

    EXPECT_NEAR(tableau.matrix[2][0], -0.5, 1e-12);
    EXPECT_NEAR(tableau.matrix[2][1], 0.0, 1e-12);
    EXPECT_NEAR(tableau.matrix[2][2], 0.0, 1e-12);
    EXPECT_NEAR(tableau.matrix[2][3], 2.5, 1e-12);
    EXPECT_NEAR(tableau.matrix[2][4], 20.0, 1e-12);

    EXPECT_EQ(tableau.basis[1], 1); // x2 is basic in row 1
}

TEST_CASE(Simplex_02_Simple2VariableLP) {
    // Hand-derived problem:
    // Max 3x1 + 5x2
    // 2x1 + x2 <= 8
    // x1 + 2x2 <= 8
    // x1, x2 >= 0
    // Optimum: x1 = 8/3 (2.66667), x2 = 8/3 (2.66667), Obj = 64/3 (21.33333)
    LPModel model("simple_2var");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x1, 2.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    EducationalSimplex solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, SimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 8.0 / 3.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[x2], 8.0 / 3.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 64.0 / 3.0, 1e-6);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(Simplex_03_OneVariableLP) {
    // Max 4x1 s.t. x1 <= 5, x1 >= 0 => x1 = 5, Obj = 20
    LPModel model("one_var");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 4.0);
    model.add_constraint("c1", {{x1, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    EducationalSimplex solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, SimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 5.0, 1e-7);
    EXPECT_NEAR(res.objective_value, 20.0, 1e-7);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(Simplex_04_OneConstraintLP) {
    // Max 2x1 + 3x2 s.t. x1 + x2 <= 10, x1, x2 >= 0 => x1=0, x2=10, Obj=30
    LPModel model("one_constraint");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 2.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 3.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 10.0);

    EducationalSimplex solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, SimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 0.0, 1e-7);
    EXPECT_NEAR(res.primal_solution[x2], 10.0, 1e-7);
    EXPECT_NEAR(res.objective_value, 30.0, 1e-7);
}

TEST_CASE(Simplex_05_MultipleConstraintLP) {
    // Max 3x1 + 2x2 + 4x3
    // x1 + x2 + 2x3 <= 4
    // 2x1 + 0.5x2 + x3 <= 5
    // 3x1 + 2x2 + x3 <= 7
    // x1, x2, x3 >= 0
    LPModel model("multi_constraint");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 2.0);
    index_t x3 = model.add_variable("x3", 0.0, BHARATOPT_INFINITY, 4.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}, {x3, 2.0}}, ConstraintSense::LESS_EQUAL, 4.0);
    model.add_constraint("c2", {{x1, 2.0}, {x2, 0.5}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);
    model.add_constraint("c3", {{x1, 3.0}, {x2, 2.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 7.0);

    EducationalSimplex solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, SimplexStatus::OPTIMAL);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
    EXPECT_NEAR(res.objective_value, 10.0, 1e-6);
}

TEST_CASE(Simplex_06_RedundantConstraintLP) {
    // Simple 2-var model with an extra redundant constraint x1 + x2 <= 20
    LPModel model("redundant_constraint");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x1, 2.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c_redundant", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 20.0);

    EducationalSimplex solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, SimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 64.0 / 3.0, 1e-6);
}

TEST_CASE(Simplex_07_MultiplePivotLP) {
    // Model requiring 3+ pivots
    LPModel model("multi_pivot");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 10.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 6.0);
    index_t x3 = model.add_variable("x3", 0.0, BHARATOPT_INFINITY, 4.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 100.0);
    model.add_constraint("c2", {{x1, 10.0}, {x2, 4.0}, {x3, 5.0}}, ConstraintSense::LESS_EQUAL, 600.0);
    model.add_constraint("c3", {{x1, 2.0}, {x2, 2.0}, {x3, 6.0}}, ConstraintSense::LESS_EQUAL, 300.0);

    EducationalSimplex solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, SimplexStatus::OPTIMAL);
    EXPECT_TRUE(res.iterations >= 2);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(Simplex_08_DegenerateLP) {
    // Degenerate LP where RHS becomes 0 for a basic variable
    LPModel model("degenerate");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 2.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 0.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, -1.0}}, ConstraintSense::LESS_EQUAL, 0.0);

    EducationalSimplex solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, SimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 0.0, 1e-7);
    EXPECT_NEAR(res.primal_solution[x2], 0.0, 1e-7);
    EXPECT_NEAR(res.objective_value, 0.0, 1e-7);
}

TEST_CASE(Simplex_09_UnboundedLP) {
    // Max x1 + x2 s.t. x1 - x2 <= 5, x1, x2 >= 0 => UNBOUNDED
    LPModel model("unbounded");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 1.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, -1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    EducationalSimplex solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, SimplexStatus::UNBOUNDED);
}

TEST_CASE(Simplex_10_InfeasibleLP) {
    // Max x1 + x2 s.t. x1 + x2 <= 2, x1 + x2 >= 5 => INFEASIBLE
    LPModel model("infeasible");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 1.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 2.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 5.0);

    EducationalSimplex solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, SimplexStatus::INFEASIBLE);
}

TEST_CASE(Simplex_11_ZeroObjectiveLP) {
    // Max 0x1 + 0x2 s.t. x1 + x2 <= 5 => Optimal obj = 0
    LPModel model("zero_objective");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 0.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 0.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    EducationalSimplex solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, SimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 0.0, 1e-7);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(Simplex_12_PhaseITwoPhaseTest) {
    // Max 2x1 + x2 s.t. x1 + x2 >= 4, x1 <= 3, x2 <= 3 => x1=3, x2=3, Obj=9
    LPModel model("phase1_test");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 2.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 1.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 4.0);
    model.add_constraint("c2", {{x1, 1.0}}, ConstraintSense::LESS_EQUAL, 3.0);
    model.add_constraint("c3", {{x2, 1.0}}, ConstraintSense::LESS_EQUAL, 3.0);

    EducationalSimplex solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, SimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 3.0, 1e-6);
    EXPECT_NEAR(res.primal_solution[x2], 3.0, 1e-6);
    EXPECT_NEAR(res.objective_value, 9.0, 1e-6);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(Simplex_13_IterationLimitTest) {
    // Set max_iterations = 1 on a 3-pivot problem
    LPModel model("iter_limit");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 10.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 6.0);
    index_t x3 = model.add_variable("x3", 0.0, BHARATOPT_INFINITY, 4.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 100.0);
    model.add_constraint("c2", {{x1, 10.0}, {x2, 4.0}, {x3, 5.0}}, ConstraintSense::LESS_EQUAL, 600.0);

    SimplexOptions options;
    options.max_iterations = 1;
    EducationalSimplex solver(options);

    SimplexResult res = solver.solve(model);
    EXPECT_EQ(res.status, SimplexStatus::ITERATION_LIMIT);
    EXPECT_EQ(res.iterations, static_cast<size_t>(1));
}

TEST_CASE(Simplex_14_NumericalStressSmallCoeff) {
    // LP with small coefficients (1e-5)
    LPModel model("numerical_stress");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 1e-5);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 2e-5);

    model.add_constraint("c1", {{x1, 1e-5}, {x2, 1e-5}}, ConstraintSense::LESS_EQUAL, 1.0);

    EducationalSimplex solver;
    SimplexResult res = solver.solve(model);

    EXPECT_EQ(res.status, SimplexStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x2], 1e5, 1e-1);
    EXPECT_TRUE(solver.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(Simplex_15_IterationTraceMode) {
    // Verify trace output recording when enable_trace is true
    LPModel model("trace_mode");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x1, 2.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    SimplexOptions options;
    options.enable_trace = true;
    EducationalSimplex solver(options);

    SimplexResult res = solver.solve(model);
    EXPECT_EQ(res.status, SimplexStatus::OPTIMAL);
    EXPECT_TRUE(res.trace.size() > 0);
    EXPECT_EQ(res.trace[0].iteration, static_cast<size_t>(1));
}

TEST_CASE(Simplex_16_PresolveSimplexPostsolvePipeline) {
    // Test the complete Phase 5 Presolve -> Phase 6 Educational Simplex -> Postsolve pipeline!
    // Original Model with fixed variable x1 = 4
    LPModel model("pipeline_test");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 4.0, 4.0, 2.0); // Fixed x1 = 4 => obj offset +8
    index_t x2 = model.add_variable("x2", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t x3 = model.add_variable("x3", 0.0, BHARATOPT_INFINITY, 5.0);

    // 2x1 + x2 + 2x3 <= 20  => x2 + 2x3 <= 12
    model.add_constraint("c1", {{x1, 2.0}, {x2, 1.0}, {x3, 2.0}}, ConstraintSense::LESS_EQUAL, 20.0);
    // x1 + 2x2 + x3 <= 16   => 2x2 + x3 <= 12
    model.add_constraint("c2", {{x1, 1.0}, {x2, 2.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 16.0);

    // Step 1: Run Presolve
    PresolveEngine presolver;
    PresolveResult presolve_res = presolver.presolve(model);

    EXPECT_EQ(presolve_res.status, PresolveStatus::SUCCESS);
    EXPECT_EQ(presolve_res.stats.vars_removed, static_cast<size_t>(1));

    // Step 2: Solve Reduced Model with Educational Simplex
    EducationalSimplex solver;
    SimplexResult simplex_res = solver.solve(presolve_res.reduced_model);

    EXPECT_EQ(simplex_res.status, SimplexStatus::OPTIMAL);

    // Step 3: Run Postsolve mapping back to Original Variable Space
    std::vector<real_t> orig_x = presolve_res.postsolve.recover_solution(simplex_res.primal_solution);

    // Step 4: Verify Original Feasibility & Objective Value
    EXPECT_NEAR(orig_x[x1], 4.0, 1e-7);
    EXPECT_TRUE(presolve_res.postsolve.verify_original_feasibility(orig_x));
    real_t orig_obj = presolve_res.postsolve.compute_original_objective(orig_x);

    // Objective value matching check
    EXPECT_NEAR(orig_obj, simplex_res.objective_value, 1e-5);
}
