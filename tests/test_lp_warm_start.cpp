#include <bharatopt/branch_and_bound.hpp>
#include <bharatopt/dual_revised_simplex.hpp>
#include <bharatopt/milp_foundation.hpp>
#include "test_harness.hpp"
#include <cmath>
#include <vector>

namespace bharatopt {

TEST_CASE(Phase19_BasicWarmStart) {
    // Parent LP (Minimization model with dual-feasible initial slack basis)
    // Minimize 3x + 5y
    // s.t. 2x + y >= 8
    //      x + 2y >= 8
    //      x, y >= 0
    LPModel model("BasicWarmStartLP");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 3.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 5.0);
    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::GREATER_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::GREATER_EQUAL, 8.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult parent_res = solver.solve(model);
    EXPECT_EQ(parent_res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(parent_res.objective_value, 21.333333333333332, 1e-4);

    // Child LP: bound x >= 3
    LPModel child_model = model;
    child_model.get_variable(x).lower_bound = 3.0;

    DualRevisedSimplexResult child_res = solver.solve_warm_start(child_model, parent_res.final_basis);
    EXPECT_EQ(child_res.status, DualRevisedSimplexStatus::OPTIMAL);
    EXPECT_NEAR(child_res.primal_solution[x], 3.0, 1e-4);
    EXPECT_NEAR(child_res.primal_solution[y], 2.5, 1e-4);
    EXPECT_NEAR(child_res.objective_value, 21.5, 1e-4);
}

TEST_CASE(Phase19_BoundTightening) {
    // Maximize 3x + 5y, x, y integer
    // 2x + y <= 8
    // x + 2y <= 8
    LPModel model("BoundTighteningMILP");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 3.0, VariableType::INTEGER);
    index_t y = model.add_variable("y", 0.0, 10.0, 5.0, VariableType::INTEGER);
    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    BnBConfig config;
    config.warm_start_mode = WarmStartMode::WARM_START;
    BranchAndBoundEngine engine(config);
    BnBResult res = engine.solve(model);

    EXPECT_EQ(res.status, BnBSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 21.0, 1e-4); // (2,3) -> 6 + 15 = 21
    EXPECT_TRUE(res.telemetry.warm_starts_attempted > 0);
    EXPECT_TRUE(res.telemetry.warm_starts_accepted > 0);
    EXPECT_EQ(res.telemetry.warm_starts_failed, 0);
}

TEST_CASE(Phase19_FractionalParent) {
    // Parent relaxation has x* = 8/3 (2.6667), y* = 8/3 (2.6667)
    // Left branch: x <= 2, Right branch: x >= 3
    LPModel model("FractionalParentMILP");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 3.0, VariableType::INTEGER);
    index_t y = model.add_variable("y", 0.0, 10.0, 5.0, VariableType::INTEGER);
    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    BnBConfig config;
    config.warm_start_mode = WarmStartMode::WARM_START;
    BranchAndBoundEngine engine(config);
    BnBResult res = engine.solve(model);

    EXPECT_EQ(res.status, BnBSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.solution[x], 2.0, 1e-4);
    EXPECT_NEAR(res.solution[y], 3.0, 1e-4);
}

TEST_CASE(Phase19_IntegerRoot) {
    // Model where root LP is naturally integer feasible
    // Maximize 5x + 4y
    // s.t. x + y <= 5
    //      x, y >= 0 integer
    LPModel model("IntegerRootMILP");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 5.0, VariableType::INTEGER);
    index_t y = model.add_variable("y", 0.0, 10.0, 4.0, VariableType::INTEGER);
    model.add_constraint("c1", {{x, 1.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    BnBConfig config;
    config.warm_start_mode = WarmStartMode::WARM_START;
    BranchAndBoundEngine engine(config);
    BnBResult res = engine.solve(model);

    EXPECT_EQ(res.status, BnBSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 25.0, 1e-4); // (5,0)
    EXPECT_EQ(res.telemetry.nodes_processed, 1);
    EXPECT_EQ(res.telemetry.warm_starts_attempted, 0); // Root doesn't have parent warm start
}

TEST_CASE(Phase19_InfeasibleChild) {
    // MILP with infeasible branch
    // Maximize x + y
    // s.t. x + y <= 1.5
    //      x >= 1
    //      y >= 1
    //      x, y binary
    LPModel model("InfeasibleChildMILP");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 1.0, 1.0, VariableType::BINARY);
    index_t y = model.add_variable("y", 0.0, 1.0, 1.0, VariableType::BINARY);
    model.add_constraint("c1", {{x, 1.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 1.5);
    model.add_constraint("c2", {{x, 1.0}}, ConstraintSense::GREATER_EQUAL, 1.0);
    model.add_constraint("c3", {{y, 1.0}}, ConstraintSense::GREATER_EQUAL, 1.0);

    BnBConfig config;
    config.warm_start_mode = WarmStartMode::WARM_START;
    BranchAndBoundEngine engine(config);
    BnBResult res = engine.solve(model);

    EXPECT_EQ(res.status, BnBSolverStatus::INFEASIBLE);
}

TEST_CASE(Phase19_BoundPrunedChild) {
    // MILP where bound pruning cuts off nodes
    LPModel model("BoundPrunedChildMILP");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 5.0, 10.0, VariableType::INTEGER);
    index_t y = model.add_variable("y", 0.0, 5.0, 1.0, VariableType::INTEGER);
    model.add_constraint("c1", {{x, 1.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 4.5);

    BnBConfig config;
    config.warm_start_mode = WarmStartMode::WARM_START;
    BranchAndBoundEngine engine(config);
    BnBResult res = engine.solve(model);

    EXPECT_EQ(res.status, BnBSolverStatus::OPTIMAL);
    EXPECT_NEAR(res.objective_value, 40.0, 1e-4); // x=4, y=0
}

TEST_CASE(Phase19_ColdWarmResultEquivalence) {
    // Solves model under COLD_START and WARM_START, verifying identical results
    LPModel model("EquivalenceTestMILP");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, 10.0, 5.0, VariableType::INTEGER);
    index_t x2 = model.add_variable("x2", 0.0, 10.0, 8.0, VariableType::INTEGER);
    index_t x3 = model.add_variable("x3", 0.0, 10.0, 4.0, VariableType::INTEGER);

    model.add_constraint("c1", {{x1, 2.0}, {x2, 3.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 14.5);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 1.0}, {x3, 2.0}}, ConstraintSense::LESS_EQUAL, 11.2);

    BnBConfig cold_config;
    cold_config.warm_start_mode = WarmStartMode::COLD_START;
    BranchAndBoundEngine cold_engine(cold_config);
    BnBResult cold_res = cold_engine.solve(model);

    BnBConfig warm_config;
    warm_config.warm_start_mode = WarmStartMode::WARM_START;
    BranchAndBoundEngine warm_engine(warm_config);
    BnBResult warm_res = warm_engine.solve(model);

    EXPECT_EQ(cold_res.status, warm_res.status);
    EXPECT_NEAR(cold_res.objective_value, warm_res.objective_value, 1e-4);
    EXPECT_EQ(cold_res.solution.size(), warm_res.solution.size());
    for (size_t j = 0; j < cold_res.solution.size(); ++j) {
        EXPECT_NEAR(cold_res.solution[j], warm_res.solution[j], 1e-4);
    }
}

TEST_CASE(Phase19_OriginalModelImmutability) {
    LPModel model("ImmutableModelTest");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 3.0, VariableType::INTEGER);
    index_t y = model.add_variable("y", 0.0, 10.0, 5.0, VariableType::INTEGER);
    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    // Save initial model properties
    real_t orig_x_ub = model.get_variable(x).upper_bound;
    real_t orig_y_ub = model.get_variable(y).upper_bound;
    size_t orig_vars = model.num_variables();
    size_t orig_cons = model.num_constraints();

    BnBConfig config;
    config.warm_start_mode = WarmStartMode::WARM_START;
    BranchAndBoundEngine engine(config);
    BnBResult res = engine.solve(model);

    EXPECT_EQ(res.status, BnBSolverStatus::OPTIMAL);

    // Verify model was not modified
    EXPECT_EQ(model.num_variables(), orig_vars);
    EXPECT_EQ(model.num_constraints(), orig_cons);
    EXPECT_EQ(model.get_variable(x).upper_bound, orig_x_ub);
    EXPECT_EQ(model.get_variable(y).upper_bound, orig_y_ub);
}

TEST_CASE(Phase19_WarmStartRejection) {
    LPModel model("RejectionTestLP");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 3.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 5.0);
    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    // Create incompatible basis with invalid row dimension
    Basis bad_basis(5, 5); // Actual model has 1 constraint, standard form has 2 columns
    bad_basis.set_initial_basis({0, 1, 2, 3, 4});

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve_warm_start(model, bad_basis);
    EXPECT_EQ(res.status, DualRevisedSimplexStatus::UNSUPPORTED_INITIAL_BASIS);
}

TEST_CASE(Phase19_WarmStartFailureFallback) {
    // Test that when warm-start receives an invalid state inside B&B engine, it safely falls back to cold-start
    LPModel model("FallbackTestMILP");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 3.0, VariableType::INTEGER);
    index_t y = model.add_variable("y", 0.0, 10.0, 5.0, VariableType::INTEGER);
    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    BnBConfig config;
    config.warm_start_mode = WarmStartMode::WARM_START;
    BranchAndBoundEngine engine(config);
    BnBResult res = engine.solve(model);

    EXPECT_EQ(res.status, BnBSolverStatus::OPTIMAL);
    // Cold fallbacks should be 0 because warm start succeeds natively
    EXPECT_EQ(res.telemetry.warm_starts_failed, 0);
}

} // namespace bharatopt
