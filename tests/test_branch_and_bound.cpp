#include "test_harness.hpp"
#include <bharatopt/branch_and_bound.hpp>
#include <cmath>

using namespace bharatopt;

// ========================================================
// HAND-DERIVED MANDATORY TEST CASES
// ========================================================

TEST_CASE(BnB_01_BinaryRelaxedConstraint_Hand1) {
    // Maximize x + y
    // Subject to:
    // 2x + 2y <= 3
    // x, y binary
    // LP relaxation: x=0.75, y=0.75, obj=1.5
    // MILP optimum: (1,0) or (0,1), obj=1.0

    LPModel model("binary_hand_1");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x_idx = model.add_variable("x", 0.0, 1.0, 1.0, VariableType::BINARY);
    index_t y_idx = model.add_variable("y", 0.0, 1.0, 1.0, VariableType::BINARY);

    model.add_constraint("c1", {{x_idx, 2.0}, {y_idx, 2.0}}, ConstraintSense::LESS_EQUAL, 3.0);

    BranchAndBoundEngine bnb;
    BnBResult res = bnb.solve(model);

    EXPECT_EQ(res.status, BnBSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 1.0, 1e-4);
    EXPECT_TRUE(res.solution[0] == 0.0 || res.solution[0] == 1.0);
    EXPECT_TRUE(res.solution[1] == 0.0 || res.solution[1] == 1.0);
    EXPECT_NEAR(res.solution[0] + res.solution[1], 1.0, 1e-4);
    EXPECT_TRUE(res.telemetry.nodes_created > 1);
}

TEST_CASE(BnB_02_2DIntegerSimplex_Hand2) {
    // Maximize x + y
    // Subject to:
    // 2x + y <= 4
    // x + 2y <= 4
    // x, y >= 0, integer
    // LP relaxation: x=4/3, y=4/3, obj=8/3 = 2.6667
    // MILP optimum: x=1, y=1, obj=2.0

    LPModel model("int_hand_2");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x_idx = model.add_variable("x", 0.0, 10.0, 1.0, VariableType::INTEGER);
    index_t y_idx = model.add_variable("y", 0.0, 10.0, 1.0, VariableType::INTEGER);

    model.add_constraint("c1", {{x_idx, 2.0}, {y_idx, 1.0}}, ConstraintSense::LESS_EQUAL, 4.0);
    model.add_constraint("c2", {{x_idx, 1.0}, {y_idx, 2.0}}, ConstraintSense::LESS_EQUAL, 4.0);

    BranchAndBoundEngine bnb;
    BnBResult res = bnb.solve(model);

    EXPECT_EQ(res.status, BnBSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 2.0, 1e-4);
    EXPECT_NEAR(res.solution[0] + res.solution[1], 2.0, 1e-4);
}

TEST_CASE(BnB_03_Phase16Model_Hand3) {
    // Maximize 3x + 5y
    // Subject to:
    // 2x + y <= 8
    // x + 2y <= 8
    // x, y >= 0, integer
    // LP relaxation: x=8/3, y=8/3, obj=64/3 = 21.3333
    // MILP optimum: (0,4) -> obj = 20.0, or (2,3) -> obj = 21.0?
    // Let's check (2,3): 2(2)+3=7 <= 8, 2+2(3)=8 <= 8. Obj = 3(2)+5(3) = 21.0!

    LPModel model("phase16_hand_3");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x_idx = model.add_variable("x", 0.0, 10.0, 3.0, VariableType::INTEGER);
    index_t y_idx = model.add_variable("y", 0.0, 10.0, 5.0, VariableType::INTEGER);

    model.add_constraint("c1", {{x_idx, 2.0}, {y_idx, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x_idx, 1.0}, {y_idx, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    BranchAndBoundEngine bnb;
    BnBResult res = bnb.solve(model);

    EXPECT_EQ(res.status, BnBSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 21.0, 1e-4);
    EXPECT_NEAR(res.solution[0], 2.0, 1e-4);
    EXPECT_NEAR(res.solution[1], 3.0, 1e-4);
}

TEST_CASE(BnB_04_RootIntegerFeasibleMILP_Hand4) {
    // Minimize x + 2y
    // Subject to:
    // x + y >= 3
    // x, y >= 0, integer
    // Root LP relaxation is x=3, y=0, obj=3.0 -> Naturally integer-feasible

    LPModel model("root_feas_hand_4");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x_idx = model.add_variable("x", 0.0, 10.0, 1.0, VariableType::INTEGER);
    index_t y_idx = model.add_variable("y", 0.0, 10.0, 2.0, VariableType::INTEGER);

    model.add_constraint("c1", {{x_idx, 1.0}, {y_idx, 1.0}}, ConstraintSense::GREATER_EQUAL, 3.0);

    BranchAndBoundEngine bnb;
    BnBResult res = bnb.solve(model);

    EXPECT_EQ(res.status, BnBSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 3.0, 1e-4);
    EXPECT_NEAR(res.solution[0], 3.0, 1e-4);
    EXPECT_NEAR(res.solution[1], 0.0, 1e-4);
    EXPECT_EQ(res.telemetry.nodes_created, static_cast<size_t>(1));
}

TEST_CASE(BnB_05_InfeasibleMILP_Hand5) {
    // Maximize x + y
    // Subject to:
    // x + y >= 5
    // x, y binary
    // Infeasible! (max x+y = 2)

    LPModel model("infeasible_hand_5");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x_idx = model.add_variable("x", 0.0, 1.0, 1.0, VariableType::BINARY);
    index_t y_idx = model.add_variable("y", 0.0, 1.0, 1.0, VariableType::BINARY);

    model.add_constraint("c1", {{x_idx, 1.0}, {y_idx, 1.0}}, ConstraintSense::GREATER_EQUAL, 5.0);

    BranchAndBoundEngine bnb;
    BnBResult res = bnb.solve(model);

    EXPECT_EQ(res.status, BnBSolverStatus::INFEASIBLE);
}

TEST_CASE(BnB_06_MinimizationMILP_Hand6) {
    // Minimize 3x + 2y
    // Subject to:
    // 2x + y >= 5
    // x + 2y >= 5
    // x, y >= 0, integer
    // LP relaxation: x=5/3, y=5/3 -> obj = 25/3 = 8.3333
    // Integer feasible points:
    // (2,1): 3(2)+2(1) = 8
    // (1,2): 3(1)+2(2) = 7 (Optimal!)

    LPModel model("min_hand_6");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x_idx = model.add_variable("x", 0.0, 10.0, 3.0, VariableType::INTEGER);
    index_t y_idx = model.add_variable("y", 0.0, 10.0, 2.0, VariableType::INTEGER);

    model.add_constraint("c1", {{x_idx, 2.0}, {y_idx, 1.0}}, ConstraintSense::GREATER_EQUAL, 5.0);
    model.add_constraint("c2", {{x_idx, 1.0}, {y_idx, 2.0}}, ConstraintSense::GREATER_EQUAL, 5.0);

    BranchAndBoundEngine bnb;
    BnBResult res = bnb.solve(model);

    EXPECT_EQ(res.status, BnBSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 9.0, 1e-4);
    EXPECT_NEAR(res.solution[0], 1.0, 1e-4);
    EXPECT_NEAR(res.solution[1], 3.0, 1e-4);
}

// ========================================================
// ADDITIONAL EDGE CASES & IMMUTABILITY TESTS
// ========================================================

TEST_CASE(BnB_07_OriginalModelImmutability) {
    LPModel model("immut_test");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x_idx = model.add_variable("x", 0.0, 10.0, 1.0, VariableType::INTEGER);
    index_t y_idx = model.add_variable("y", 0.0, 10.0, 1.0, VariableType::INTEGER);

    model.add_constraint("c1", {{x_idx, 2.0}, {y_idx, 1.0}}, ConstraintSense::LESS_EQUAL, 4.0);

    BranchAndBoundEngine bnb;
    BnBResult res = bnb.solve(model);

    EXPECT_EQ(res.status, BnBSolverStatus::OPTIMAL);

    // Verify original model remains 100% unchanged
    EXPECT_EQ(model.variables()[0].type, VariableType::INTEGER);
    EXPECT_NEAR(model.variables()[0].lower_bound, 0.0, 1e-9);
    EXPECT_NEAR(model.variables()[0].upper_bound, 10.0, 1e-9);
    EXPECT_EQ(model.variables()[1].type, VariableType::INTEGER);
    EXPECT_NEAR(model.variables()[1].lower_bound, 0.0, 1e-9);
    EXPECT_NEAR(model.variables()[1].upper_bound, 10.0, 1e-9);
}

TEST_CASE(BnB_08_NodeLimitReached) {
    LPModel model("node_limit_test");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x_idx = model.add_variable("x", 0.0, 10.0, 3.0, VariableType::INTEGER);
    index_t y_idx = model.add_variable("y", 0.0, 10.0, 5.0, VariableType::INTEGER);

    model.add_constraint("c1", {{x_idx, 2.0}, {y_idx, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x_idx, 1.0}, {y_idx, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    BnBConfig cfg;
    cfg.max_nodes = 2; // Strict node limit
    BranchAndBoundEngine bnb(cfg);

    BnBResult res = bnb.solve(model);
    EXPECT_EQ(res.status, BnBSolverStatus::LIMIT_REACHED);
}
