#include "test_harness.hpp"
#include <bharatopt/lp_model.hpp>

using namespace bharatopt;

TEST_CASE(SmallSpecifiedModelTest) {
    // Maximise: 3x + 5y
    // subject to: 2x + y <= 8
    //            x + 2y <= 8
    //            x >= 0, y >= 0
    LPModel model("small_test_model");
    model.set_sense(ObjectiveSense::MAXIMIZE);

    index_t x_idx = model.add_variable("x", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t y_idx = model.add_variable("y", 0.0, BHARATOPT_INFINITY, 5.0);

    EXPECT_EQ(model.num_variables(), static_cast<size_t>(2));
    EXPECT_EQ(model.sense(), ObjectiveSense::MAXIMIZE);

    // 2x + y <= 8
    model.add_constraint("c1", {{x_idx, 2.0}, {y_idx, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    // x + 2y <= 8
    model.add_constraint("c2", {{x_idx, 1.0}, {y_idx, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    EXPECT_EQ(model.num_constraints(), static_cast<size_t>(2));
    EXPECT_EQ(model.num_nonzeros(), static_cast<size_t>(4));

    EXPECT_NEAR(model.get_coeff(0, x_idx), 2.0, 1e-9);
    EXPECT_NEAR(model.get_coeff(0, y_idx), 1.0, 1e-9);
    EXPECT_NEAR(model.get_coeff(1, x_idx), 1.0, 1e-9);
    EXPECT_NEAR(model.get_coeff(1, y_idx), 2.0, 1e-9);
}

TEST_CASE(MinimizationAndObjectiveOffsetTest) {
    LPModel model("minimization_model");
    model.set_sense(ObjectiveSense::MINIMIZE);
    model.set_obj_offset(10.5);

    index_t x1 = model.add_variable("x1", -5.0, 10.0, 2.5);
    index_t x2 = model.add_variable("x2", 0.0, 100.0, -1.0);

    EXPECT_EQ(model.sense(), ObjectiveSense::MINIMIZE);
    EXPECT_NEAR(model.obj_offset(), 10.5, 1e-9);
    EXPECT_NEAR(model.get_variable(x1).obj_coeff, 2.5, 1e-9);
    EXPECT_NEAR(model.get_variable(x2).obj_coeff, -1.0, 1e-9);
}

TEST_CASE(ConstraintSensesTest) {
    LPModel model("senses_test");
    index_t v1 = model.add_variable("v1", 0.0, 10.0);
    index_t v2 = model.add_variable("v2", 0.0, 10.0);

    // <= constraint
    index_t c_le = model.add_constraint("c_le", {{v1, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);
    // = constraint
    index_t c_eq = model.add_constraint("c_eq", {{v1, 1.0}, {v2, -1.0}}, ConstraintSense::EQUAL, 0.0);
    // >= constraint
    index_t c_ge = model.add_constraint("c_ge", {{v2, 2.0}}, ConstraintSense::GREATER_EQUAL, 4.0);
    // RANGED constraint
    index_t c_rg = model.add_constraint("c_rg", {{v1, 1.0}, {v2, 1.0}}, ConstraintSense::RANGED, 2.0, 10.0);

    EXPECT_EQ(model.get_constraint(c_le).sense, ConstraintSense::LESS_EQUAL);
    EXPECT_EQ(model.get_constraint(c_eq).sense, ConstraintSense::EQUAL);
    EXPECT_EQ(model.get_constraint(c_ge).sense, ConstraintSense::GREATER_EQUAL);
    EXPECT_EQ(model.get_constraint(c_rg).sense, ConstraintSense::RANGED);

    EXPECT_NEAR(model.get_constraint(c_rg).rhs, 2.0, 1e-9);
    EXPECT_NEAR(model.get_constraint(c_rg).range_upper, 10.0, 1e-9);
}

TEST_CASE(VariableBoundsAndTypesTest) {
    LPModel model("bounds_test");

    index_t free_var = model.add_variable("x_free", -BHARATOPT_INFINITY, BHARATOPT_INFINITY);
    index_t fixed_var = model.add_variable("x_fixed", 4.0, 4.0);
    index_t bounded_var = model.add_variable("x_bounded", 0.0, 10.0);

    EXPECT_TRUE(model.get_variable(free_var).is_free());
    EXPECT_TRUE(model.get_variable(fixed_var).is_fixed());
    EXPECT_FALSE(model.get_variable(bounded_var).is_free());
    EXPECT_FALSE(model.get_variable(bounded_var).is_fixed());
}

TEST_CASE(EmptyModelTest) {
    LPModel model("empty");
    EXPECT_EQ(model.num_variables(), static_cast<size_t>(0));
    EXPECT_EQ(model.num_constraints(), static_cast<size_t>(0));
    EXPECT_EQ(model.num_nonzeros(), static_cast<size_t>(0));

    model.clear();
    EXPECT_EQ(model.name(), "unnamed_model");
}

TEST_CASE(InvalidModelQueriesTest) {
    LPModel model("invalid_queries");
    index_t v0 = model.add_variable("v0", 0.0, 10.0);
    (void)v0;

    // Duplicate variable name
    bool caught_dup_var = false;
    try {
        model.add_variable("v0", 0.0, 10.0);
    } catch (const std::invalid_argument&) {
        caught_dup_var = true;
    }
    EXPECT_TRUE(caught_dup_var);

    // Out of range index lookup
    bool caught_range_var = false;
    try {
        (void)model.get_variable(99);
    } catch (const std::out_of_range&) {
        caught_range_var = true;
    }
    EXPECT_TRUE(caught_range_var);

    // Non-existent name lookup
    bool caught_name_var = false;
    try {
        (void)model.get_variable("non_existent");
    } catch (const std::invalid_argument&) {
        caught_name_var = true;
    }
    EXPECT_TRUE(caught_name_var);

    // Invalid variable index in constraint
    bool caught_invalid_term = false;
    try {
        model.add_constraint("c_bad", {{99, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);
    } catch (const std::out_of_range&) {
        caught_invalid_term = true;
    }
    EXPECT_TRUE(caught_invalid_term);
}
