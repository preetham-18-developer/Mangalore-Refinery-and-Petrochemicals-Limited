#include "test_harness.hpp"
#include <bharatopt/lp_model.hpp>
#include <bharatopt/presolve.hpp>
#include <cmath>

using namespace bharatopt;

TEST_CASE(Presolve_01_NoOpModel) {
    LPModel model("no_op");
    index_t x1 = model.add_variable("x1", 0.0, 10.0, 3.0);
    index_t x2 = model.add_variable("x2", 0.0, 10.0, 5.0);

    model.add_constraint("c1", {{x1, 2.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x1, 1.0}, {x2, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::NO_CHANGE);
    EXPECT_EQ(res.stats.vars_removed, static_cast<size_t>(0));
    EXPECT_EQ(res.stats.cons_removed, static_cast<size_t>(0));
}

TEST_CASE(Presolve_02_FixedVariableElimination) {
    // x1 fixed at 3 (lb = ub = 3)
    // 2x1 + x2 <= 10 => x2 <= 4. Since min 5x2 with x2 >= 0, x2 fixed at 0.
    // Presolve solves model completely (0 vars remaining).
    LPModel model("fixed_var_test");
    index_t x1 = model.add_variable("x1", 3.0, 3.0, 2.0); // Fixed x1 = 3
    index_t x2 = model.add_variable("x2", 0.0, 10.0, 5.0);

    model.add_constraint("c1", {{x1, 2.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 10.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::SUCCESS);
    EXPECT_EQ(res.reduced_model.num_variables(), static_cast<size_t>(0));

    // Postsolve reconstruction from empty reduced solution {}
    std::vector<real_t> reduced_x = {};
    std::vector<real_t> orig_x = res.postsolve.recover_solution(reduced_x);

    EXPECT_NEAR(orig_x[x1], 3.0, 1e-12); // x1 = 3
    EXPECT_NEAR(orig_x[x2], 0.0, 1e-12); // x2 = 0

    // Check feasibility & objective in original space
    EXPECT_TRUE(res.postsolve.verify_original_feasibility(orig_x));
    // Objective: 2*3 + 5*0 = 6
    EXPECT_NEAR(res.postsolve.compute_original_objective(orig_x), 6.0, 1e-12);
}

TEST_CASE(Presolve_03_MultipleFixedVariables) {
    LPModel model("multi_fixed");
    index_t x1 = model.add_variable("x1", 2.0, 2.0, 1.0);
    index_t x2 = model.add_variable("x2", -5.0, -5.0, 3.0);
    index_t x3 = model.add_variable("x3", 0.0, 10.0, 4.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 0.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::SUCCESS);

    std::vector<real_t> orig_x = res.postsolve.recover_solution({});
    EXPECT_NEAR(orig_x[x1], 2.0, 1e-12);
    EXPECT_NEAR(orig_x[x2], -5.0, 1e-12);
    EXPECT_NEAR(orig_x[x3], 0.0, 1e-12);

    EXPECT_TRUE(res.postsolve.verify_original_feasibility(orig_x));
}

TEST_CASE(Presolve_04_EmptyRowRedundant) {
    LPModel model("empty_row_redundant");
    model.add_variable("x1", 0.0, 5.0, 1.0);
    model.add_constraint("c_empty", {}, ConstraintSense::LESS_EQUAL, 10.0); // 0 <= 10 (Redundant)

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::SUCCESS);
    EXPECT_EQ(res.stats.cons_removed, static_cast<size_t>(1));
}

TEST_CASE(Presolve_05_EmptyRowInfeasible) {
    LPModel model("empty_row_infeasible");
    model.add_variable("x1", 0.0, 5.0, 1.0);
    model.add_constraint("c_infeas", {}, ConstraintSense::LESS_EQUAL, -5.0); // 0 <= -5 (Infeasible)

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::INFEASIBLE);
}

TEST_CASE(Presolve_06_EmptyColumnBounded) {
    LPModel model("empty_col_bounded");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x1 = model.add_variable("x1", 0.0, 10.0, 2.0); // No constraints, min +2*x1 -> fix x1 = 0
    index_t x2 = model.add_variable("x2", 0.0, 10.0, 1.0);
    index_t x3 = model.add_variable("x3", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{x2, 1.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::SUCCESS);
    EXPECT_EQ(res.reduced_model.num_variables(), static_cast<size_t>(2)); // x1 removed, x2 and x3 remain

    std::vector<real_t> orig_x = res.postsolve.recover_solution({1.0, 2.0});
    EXPECT_NEAR(orig_x[x1], 0.0, 1e-12);
    EXPECT_NEAR(orig_x[x2], 1.0, 1e-12);
    EXPECT_NEAR(orig_x[x3], 2.0, 1e-12);
}

TEST_CASE(Presolve_07_EmptyColumnUnbounded) {
    LPModel model("empty_col_unbounded");
    model.set_sense(ObjectiveSense::MINIMIZE);
    // Free variable with c1 = -2 in MINIMIZE model -> Unbounded (-infty)
    model.add_variable("x1", -BHARATOPT_INFINITY, BHARATOPT_INFINITY, -2.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::UNBOUNDED);
}

TEST_CASE(Presolve_08_SingletonRow) {
    LPModel model("singleton_row");
    index_t x1 = model.add_variable("x1", 0.0, 10.0, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, 10.0, 1.0);
    // 2x1 <= 8 => x1 <= 4 (Singleton)
    model.add_constraint("c_single", {{x1, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    // x1 + x2 <= 10 (Keeps x1 active)
    model.add_constraint("c_multi", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 10.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::SUCCESS);
    // Upper bound tightened to 4.0
    EXPECT_NEAR(res.reduced_model.get_variable("x1").upper_bound, 4.0, 1e-12);
    EXPECT_EQ(res.reduced_model.num_constraints(), static_cast<size_t>(1));
}

TEST_CASE(Presolve_09_BoundTightening) {
    LPModel model("bound_tightening");
    index_t x1 = model.add_variable("x1", 0.0, 10.0, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{x1, 1.0}}, ConstraintSense::GREATER_EQUAL, 3.0); // x1 >= 3
    model.add_constraint("c2", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 12.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::SUCCESS);
    EXPECT_NEAR(res.reduced_model.get_variable("x1").lower_bound, 3.0, 1e-12);
}

TEST_CASE(Presolve_10_ContradictoryBounds) {
    LPModel model("contradictory_bounds");
    // x1 in [0, 2], but constraint requires x1 >= 5 -> Infeasible
    index_t x1 = model.add_variable("x1", 0.0, 2.0, 1.0);
    model.add_constraint("c1", {{x1, 1.0}}, ConstraintSense::GREATER_EQUAL, 5.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::INFEASIBLE);
}

TEST_CASE(Presolve_11_RedundantConstraint) {
    LPModel model("redundant_cons");
    index_t x1 = model.add_variable("x1", 0.0, 2.0, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, 2.0, 1.0);
    // 0x1 + 0x2 <= 0
    model.add_constraint("c_red", {{x1, 0.0}, {x2, 0.0}}, ConstraintSense::LESS_EQUAL, 0.0);
    model.add_constraint("c_valid", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 3.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::SUCCESS);
    EXPECT_EQ(res.stats.cons_removed, static_cast<size_t>(1));
}

TEST_CASE(Presolve_12_MultipleSimultaneousReductions) {
    LPModel model("multi_reductions");
    index_t x1 = model.add_variable("x1", 5.0, 5.0, 2.0); // Fixed x1 = 5
    index_t x2 = model.add_variable("x2", 0.0, 10.0, 1.0);
    index_t x3 = model.add_variable("x3", 0.0, 10.0, 1.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 2.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 20.0); // 2x2 + x3 <= 15
    model.add_constraint("c_empty", {}, ConstraintSense::LESS_EQUAL, 100.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::SUCCESS);
    EXPECT_EQ(res.stats.vars_removed, static_cast<size_t>(1)); // x1 fixed
    EXPECT_EQ(res.stats.cons_removed, static_cast<size_t>(1)); // c_empty removed
}

TEST_CASE(Presolve_13_ObjectiveConstantPreservation) {
    LPModel model("obj_const_preservation");
    model.set_obj_offset(50.0);
    index_t x1 = model.add_variable("x1", 4.0, 4.0, 3.0); // x1 = 4 fixed => obj += 12
    (void)x1;
    index_t x2 = model.add_variable("x2", 0.0, 10.0, 1.0);
    index_t x3 = model.add_variable("x3", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{x2, 1.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::SUCCESS);
    // Reduced model objective offset = 50 + 12 = 62
    EXPECT_NEAR(res.reduced_model.obj_offset(), 62.0, 1e-12);
}

TEST_CASE(Presolve_14_VariableMapping) {
    LPModel model("var_mapping");
    model.add_variable("x1", 2.0, 2.0); // Fixed (removed)
    index_t x2 = model.add_variable("x2", 0.0, 10.0);
    index_t x3 = model.add_variable("x3", 0.0, 10.0);
    model.add_constraint("c1", {{x2, 1.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.reduced_model.num_variables(), static_cast<size_t>(2));
    EXPECT_EQ(res.reduced_model.get_variable(0).name, "x2");
    EXPECT_EQ(res.reduced_model.get_variable(1).name, "x3");
}

TEST_CASE(Presolve_15_PostsolveReconstruction) {
    LPModel model("postsolve_reconstruction");
    index_t x1 = model.add_variable("x1", 10.0, 10.0);
    index_t x2 = model.add_variable("x2", 0.0, 5.0);
    index_t x3 = model.add_variable("x3", 0.0, 5.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 18.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    // Reduced vars: x2 (idx 0), x3 (idx 1)
    std::vector<real_t> reduced_x = {1.5, 2.5}; // x2 = 1.5, x3 = 2.5
    std::vector<real_t> orig_x = res.postsolve.recover_solution(reduced_x);

    EXPECT_NEAR(orig_x[x1], 10.0, 1e-12);
    EXPECT_NEAR(orig_x[x2], 1.5, 1e-12);
    EXPECT_NEAR(orig_x[x3], 2.5, 1e-12);
}

TEST_CASE(Presolve_16_OriginalFeasibilityAfterPostsolve) {
    LPModel model("feasibility_postsolve");
    index_t x1 = model.add_variable("x1", 2.0, 2.0);
    index_t x2 = model.add_variable("x2", 0.0, 10.0);
    index_t x3 = model.add_variable("x3", 0.0, 10.0);

    model.add_constraint("c1", {{x1, 3.0}, {x2, 2.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 20.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    std::vector<real_t> orig_x = res.postsolve.recover_solution({4.0, 2.0}); // x2 = 4, x3 = 2
    EXPECT_TRUE(res.postsolve.verify_original_feasibility(orig_x));
}

TEST_CASE(Presolve_17_OriginalObjectiveAfterPostsolve) {
    LPModel model("objective_postsolve");
    model.set_obj_offset(100.0);
    index_t x1 = model.add_variable("x1", 3.0, 3.0, 5.0); // fixed x1=3 => obj += 15
    (void)x1;
    index_t x2 = model.add_variable("x2", 0.0, 10.0, 2.0);
    index_t x3 = model.add_variable("x3", 0.0, 10.0, 1.0);

    model.add_constraint("c1", {{x2, 1.0}, {x3, 1.0}}, ConstraintSense::LESS_EQUAL, 12.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    std::vector<real_t> orig_x = res.postsolve.recover_solution({4.0, 3.0}); // x2 = 4, x3 = 3
    real_t orig_obj = res.postsolve.compute_original_objective(orig_x);

    // Objective: 100 + 5*3 + 2*4 + 1*3 = 126.0
    EXPECT_NEAR(orig_obj, 126.0, 1e-12);
}

TEST_CASE(Presolve_18_ModelNoReductionsPossible) {
    LPModel model("no_reductions");
    index_t x1 = model.add_variable("x1", 0.0, 10.0, 1.0);
    index_t x2 = model.add_variable("x2", 0.0, 10.0, 1.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x1, 2.0}, {x2, 3.0}}, ConstraintSense::GREATER_EQUAL, 2.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::NO_CHANGE);
}

TEST_CASE(Presolve_19_MixedReductionModel) {
    LPModel model("mixed_reductions");
    index_t x1 = model.add_variable("x1", 4.0, 4.0, 3.0); // Fixed
    index_t x2 = model.add_variable("x2", 0.0, 10.0, 2.0);
    index_t x3 = model.add_variable("x3", 0.0, 10.0, 0.0); // Empty column
    (void)x3;
    index_t x4 = model.add_variable("x4", 0.0, 10.0, 1.0);

    model.add_constraint("c1", {{x1, 1.0}, {x2, 2.0}, {x4, 1.0}}, ConstraintSense::LESS_EQUAL, 14.0);
    model.add_constraint("c2", {}, ConstraintSense::LESS_EQUAL, 50.0); // Empty row

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::SUCCESS);
    EXPECT_TRUE(res.stats.vars_removed >= 2);
    EXPECT_TRUE(res.stats.cons_removed >= 1);
}

TEST_CASE(Presolve_20_NumericalEdgeCases) {
    LPModel model("numerical_edge");
    // Near-zero bound gap within tolerance
    model.add_variable("x1", 1.0000000000001, 1.0000000000002, 5.0);

    PresolveEngine engine;
    PresolveResult res = engine.presolve(model);

    EXPECT_EQ(res.status, PresolveStatus::SUCCESS);
    EXPECT_EQ(res.stats.vars_removed, static_cast<size_t>(1));
}
