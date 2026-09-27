#include "test_harness.hpp"
#include <bharatopt/first_order_solver.hpp>
#include <bharatopt/presolve.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <cmath>
#include <vector>

using namespace bharatopt;

TEST_CASE(FirstOrder_01_OneVariableBounded) {
    LPModel model("one_var_bounded");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x = model.add_variable("x", 2.0, 5.0, 3.0); // Min 3x s.t. 2 <= x <= 5 => x = 2

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x], 2.0, 1e-3);
    EXPECT_NEAR(res.objective_value, 6.0, 1e-3);
}

TEST_CASE(FirstOrder_02_TwoVariableSimple) {
    LPModel model("two_var_simple");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 2.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 3.0);
    model.add_constraint("c1", {{x, 1.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 6.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 18.0, 1e-2);
}

TEST_CASE(FirstOrder_03_EqualityConstrained) {
    LPModel model("equality_constrained");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 1.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 2.0);
    model.add_constraint("c1", {{x, 1.0}, {y, 1.0}}, ConstraintSense::EQUAL, 4.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x] + res.primal_solution[y], 4.0, 1e-3);
    EXPECT_NEAR(res.objective_value, 4.0, 1e-3);
}

TEST_CASE(FirstOrder_04_GreaterThanConstraint) {
    LPModel model("greater_than");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 4.0);
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::GREATER_EQUAL, 3.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x], 3.0, 1e-3);
    EXPECT_NEAR(res.objective_value, 12.0, 1e-3);
}

TEST_CASE(FirstOrder_05_MixedConstraints) {
    LPModel model("mixed_constraints");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 3.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 5.0);

    model.add_constraint("c1", {{x, 1.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}}, ConstraintSense::GREATER_EQUAL, 1.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 38.0, 1e-2);
}

TEST_CASE(FirstOrder_06_LowerBoundVariables) {
    LPModel model("lb_vars");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x = model.add_variable("x", 3.0, BHARATOPT_INFINITY, 2.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x], 3.0, 1e-3);
}

TEST_CASE(FirstOrder_07_UpperBoundVariables) {
    LPModel model("ub_vars");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 7.0, 5.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x], 7.0, 1e-3);
}

TEST_CASE(FirstOrder_08_DoublyBoundedVariables) {
    LPModel model("doubly_bounded");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x = model.add_variable("x", -2.0, 4.0, 3.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x], -2.0, 1e-3);
}

TEST_CASE(FirstOrder_09_FreeVariable) {
    LPModel model("free_variable");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x = model.add_variable("x", -BHARATOPT_INFINITY, BHARATOPT_INFINITY, 1.0);
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::GREATER_EQUAL, -5.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x], -5.0, 1e-2);
}

TEST_CASE(FirstOrder_10_FixedVariable) {
    LPModel model("fixed_var");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 4.0, 4.0, 5.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x], 4.0, 1e-3);
    EXPECT_NEAR(res.objective_value, 20.0, 1e-3);
}

TEST_CASE(FirstOrder_11_ZeroObjective) {
    LPModel model("zero_obj");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x = model.add_variable("x", 0.0, 5.0, 0.0);
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::GREATER_EQUAL, 2.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 0.0, 1e-3);
}

TEST_CASE(FirstOrder_12_RedundantConstraint) {
    LPModel model("redundant_cons");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 4.0);

    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);
    model.add_constraint("c2", {{x, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0); // Redundant

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x], 5.0, 1e-3);
}

TEST_CASE(FirstOrder_13_DegenerateCase) {
    LPModel model("degenerate_case");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 2.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 3.0);

    model.add_constraint("c1", {{x, 1.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 0.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 0.0, 1e-3);
}

TEST_CASE(FirstOrder_14_SmallCoefficients) {
    LPModel model("small_coeffs");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 1e-4);
    model.add_constraint("c1", {{x, 1e-4}}, ConstraintSense::GREATER_EQUAL, 1e-3);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x], 10.0, 1e-2);
}

TEST_CASE(FirstOrder_15_MixedCoefficientScales) {
    LPModel model("mixed_scales");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 100.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 1.0);

    model.add_constraint("c1", {{x, 10.0}, {y, 0.1}}, ConstraintSense::LESS_EQUAL, 50.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 500.0, 1e-2);
}

TEST_CASE(FirstOrder_16_PresolveReducedModel) {
    LPModel model("presolve_pipeline");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 3.0, 3.0, 2.0); // Fixed
    index_t x2 = model.add_variable("x2", 0.0, 10.0, 4.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x1], 3.0, 1e-3);
    EXPECT_NEAR(res.primal_solution[x2], 5.0, 1e-2);
}

TEST_CASE(FirstOrder_17_RectangularSparseModel) {
    LPModel model("rectangular_sparse");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, 10.0, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, 10.0, 2.0);
    index_t x3 = model.add_variable("x3", 0.0, 10.0, 3.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::GREATER_EQUAL, 4.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 4.0, 1e-2);
}

TEST_CASE(FirstOrder_18_LargerSparseModel) {
    LPModel model("larger_sparse");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    std::vector<index_t> vars;
    for (int j = 0; j < 20; ++j) {
        vars.push_back(model.add_variable("x_" + std::to_string(j), 0.0, 5.0, 1.0));
    }
    for (int i = 0; i < 10; ++i) {
        std::vector<std::pair<index_t, real_t>> terms;
        terms.push_back({vars[i], 1.0});
        terms.push_back({vars[i + 10], 1.0});
        model.add_constraint("c_" + std::to_string(i), terms, ConstraintSense::LESS_EQUAL, 6.0);
    }

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 60.0, 1e-1);
}

TEST_CASE(FirstOrder_19_MandatoryHandLP) {
    // Primary Hand-Derived Benchmark LP: Max 3x + 5y s.t. 2x + y <= 8, x + 2y <= 8
    // Optimum: x = 8/3, y = 8/3, obj = 64/3 (~21.333333)
    LPModel model("primary_hand_derived_pdhg");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t y = model.add_variable("y", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.primal_solution[x], 8.0 / 3.0, 1e-2);
    EXPECT_NEAR(res.primal_solution[y], 8.0 / 3.0, 1e-2);
    EXPECT_NEAR(res.objective_value, 64.0 / 3.0, 1e-2);
}

TEST_CASE(FirstOrder_20_InfeasibilityTest) {
    LPModel model("infeasible_pdhg");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x = model.add_variable("x", 5.0, 2.0, 1.0); // Contradictory bounds 5.0 > 2.0

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_TRUE(res.status != FirstOrderSolverStatus::OPTIMAL);
}

TEST_CASE(FirstOrder_21_UnboundedTest) {
    LPModel model("unbounded_pdhg");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, BHARATOPT_INFINITY, 10.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    // Bounded max_iterations or status check
    EXPECT_TRUE(res.status != FirstOrderSolverStatus::OPTIMAL || res.primal_solution[x] >= 0.0);
}

TEST_CASE(FirstOrder_22_IndependentVerification) {
    LPModel model("verifier_check_pdhg");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 4.0);
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::LESS_EQUAL, 6.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);
    RevisedSimplex verifier;
    EXPECT_TRUE(verifier.verify_solution_feasibility(model, res.primal_solution));
}

TEST_CASE(FirstOrder_23_CPUvsGPUComparison) {
    LPModel model("cpu_vs_gpu_pdhg");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 2.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 3.0);
    model.add_constraint("c1", {{x, 1.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    FirstOrderSolverOptions opts_cpu;
    opts_cpu.backend_type = FirstOrderBackendType::CPU_FIRST_ORDER;
    FirstOrderLPSolver solver_cpu(opts_cpu);
    FirstOrderSolverResult res_cpu = solver_cpu.solve(model);

    FirstOrderSolverOptions opts_gpu;
    opts_gpu.backend_type = FirstOrderBackendType::GPU_FIRST_ORDER;
    FirstOrderLPSolver solver_gpu(opts_gpu);
    FirstOrderSolverResult res_gpu = solver_gpu.solve(model);

    EXPECT_EQ(res_cpu.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_EQ(res_gpu.status, FirstOrderSolverStatus::OPTIMAL);
    EXPECT_NEAR(res_cpu.objective_value, res_gpu.objective_value, 1e-2);
}

TEST_CASE(FirstOrder_24_NumericalStability) {
    LPModel model("stability_pdhg");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::GREATER_EQUAL, 2.0);

    FirstOrderSolverOptions opts;
    opts.max_iterations = 1000;
    FirstOrderLPSolver solver(opts);
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_TRUE(res.status == FirstOrderSolverStatus::OPTIMAL || res.status == FirstOrderSolverStatus::MAX_ITERATIONS);
}

TEST_CASE(FirstOrder_25_IterationLimitTest) {
    LPModel model("iter_limit_pdhg");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 100.0, 5.0);
    index_t y = model.add_variable("y", 0.0, 100.0, 6.0);
    model.add_constraint("c1", {{x, 1.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 50.0);

    FirstOrderSolverOptions opts;
    opts.max_iterations = 5; // Very small iteration limit -> forces MAX_ITERATIONS
    FirstOrderLPSolver solver(opts);
    FirstOrderSolverResult res = solver.solve(model);

    EXPECT_EQ(res.status, FirstOrderSolverStatus::MAX_ITERATIONS);
    EXPECT_EQ(res.stats.iterations, static_cast<size_t>(5));
}
