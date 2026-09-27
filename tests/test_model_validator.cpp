#include "test_harness.hpp"
#include <bharatopt/lp_model.hpp>
#include <bharatopt/model_validator.hpp>
#include <cmath>

using namespace bharatopt;

// ============================================================================
// 20 VALID MODEL TESTS
// ============================================================================

TEST_CASE(Valid_01_EmptyModel) {
    LPModel model("valid_empty");
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_TRUE(res.is_valid());
    EXPECT_EQ(res.error_count(), static_cast<size_t>(0));
}

TEST_CASE(Valid_02_SingleVariable) {
    LPModel model("single_var");
    model.add_variable("x", 0.0, 10.0, 1.0);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_03_SingleConstraint) {
    LPModel model("single_cons");
    index_t x = model.add_variable("x", 0.0, 10.0);
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_04_MultipleVariables) {
    LPModel model("multi_var");
    model.add_variable("x1", 0.0, 10.0);
    model.add_variable("x2", -1.0, 1.0);
    model.add_variable("x3", 0.0, BHARATOPT_INFINITY);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_05_MultipleConstraints) {
    LPModel model("multi_cons");
    index_t x1 = model.add_variable("x1", 0.0, 10.0);
    index_t x2 = model.add_variable("x2", 0.0, 10.0);
    model.add_constraint("c1", {{x1, 1.0}, {x2, 2.0}}, ConstraintSense::LESS_EQUAL, 15.0);
    model.add_constraint("c2", {{x1, 3.0}, {x2, -1.0}}, ConstraintSense::GREATER_EQUAL, 2.0);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_06_Minimization) {
    LPModel model("minimization");
    model.set_sense(ObjectiveSense::MINIMIZE);
    model.add_variable("x", 0.0, 5.0, -3.0);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_07_Maximization) {
    LPModel model("maximization");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    model.add_variable("x", 0.0, 5.0, 3.0);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_08_LessEqualConstraint) {
    LPModel model("le_cons");
    index_t x = model.add_variable("x", 0.0, 5.0);
    model.add_constraint("c_le", {{x, 2.0}}, ConstraintSense::LESS_EQUAL, 10.0);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_09_GreaterEqualConstraint) {
    LPModel model("ge_cons");
    index_t x = model.add_variable("x", 0.0, 5.0);
    model.add_constraint("c_ge", {{x, 2.0}}, ConstraintSense::GREATER_EQUAL, 1.0);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_10_EqualityConstraint) {
    LPModel model("eq_cons");
    index_t x = model.add_variable("x", 0.0, 5.0);
    model.add_constraint("c_eq", {{x, 1.0}}, ConstraintSense::EQUAL, 3.0);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_11_RangedConstraint) {
    LPModel model("ranged_cons");
    index_t x = model.add_variable("x", 0.0, 10.0);
    model.add_constraint("c_rg", {{x, 1.0}}, ConstraintSense::RANGED, 2.0, 8.0);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_12_FiniteVariableBounds) {
    LPModel model("finite_bounds");
    model.add_variable("x", -10.0, 10.0);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_13_LowerBoundedVariable) {
    LPModel model("lb_bounded");
    model.add_variable("x", 5.0, BHARATOPT_INFINITY);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_14_UpperBoundedVariable) {
    LPModel model("ub_bounded");
    model.add_variable("x", -BHARATOPT_INFINITY, 10.0);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_15_FreeVariable) {
    LPModel model("free_var");
    model.add_variable("x", -BHARATOPT_INFINITY, BHARATOPT_INFINITY);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_16_FixedVariable) {
    LPModel model("fixed_var");
    model.add_variable("x", 4.2, 4.2);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_17_ContinuousVariable) {
    LPModel model("cont_var");
    model.add_variable("x", 0.0, 1.0, 1.0, VariableType::CONTINUOUS);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_18_IntegerVariable) {
    LPModel model("int_var");
    model.add_variable("y", 0.0, 10.0, 1.0, VariableType::INTEGER);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_19_BinaryVariable) {
    LPModel model("bin_var");
    model.add_variable("z", 0.0, 1.0, 1.0, VariableType::BINARY);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Valid_20_MixedModel) {
    LPModel model("mixed_model");
    index_t x = model.add_variable("x", 0.0, BHARATOPT_INFINITY, 2.0, VariableType::CONTINUOUS);
    index_t y = model.add_variable("y", 0.0, 10.0, 5.0, VariableType::INTEGER);
    index_t z = model.add_variable("z", 0.0, 1.0, 1.0, VariableType::BINARY);

    model.add_constraint("c1", {{x, 1.0}, {y, 2.0}, {z, 0.5}}, ConstraintSense::LESS_EQUAL, 20.0);
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

// ============================================================================
// 20 INVALID / DIAGNOSTIC MODEL TESTS
// ============================================================================

TEST_CASE(Invalid_01_LowerBoundGreaterThanUpperBound) {
    LPModel model("inv_lb_ub");
    model.add_variable("x", 10.0, 5.0); // 10 > 5
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
    EXPECT_TRUE(res.error_count() > 0);
    EXPECT_EQ(res.errors()[0].category, "VARIABLE");
}

TEST_CASE(Invalid_02_BinaryBoundsOutOfRange) {
    LPModel model("inv_bin");
    model.add_variable("z", -1.0, 2.0, 1.0, VariableType::BINARY);
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
    EXPECT_EQ(res.errors()[0].category, "VARIABLE");
}

TEST_CASE(Invalid_03_NaNVariableLowerBound) {
    LPModel model("nan_lb");
    model.add_variable("x", std::numeric_limits<real_t>::quiet_NaN(), 10.0);
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
}

TEST_CASE(Invalid_04_NaNVariableUpperBound) {
    LPModel model("nan_ub");
    model.add_variable("x", 0.0, std::numeric_limits<real_t>::quiet_NaN());
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
}

TEST_CASE(Invalid_05_NaNCoefficient) {
    LPModel model("nan_coeff");
    index_t x = model.add_variable("x", 0.0, 10.0);
    model.add_constraint("c1", {{x, std::numeric_limits<real_t>::quiet_NaN()}}, ConstraintSense::LESS_EQUAL, 5.0);
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
    EXPECT_EQ(res.errors()[0].category, "COEFFICIENT");
}

TEST_CASE(Invalid_06_NaNRHS) {
    LPModel model("nan_rhs");
    index_t x = model.add_variable("x", 0.0, 10.0);
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::LESS_EQUAL, std::numeric_limits<real_t>::quiet_NaN());
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
    EXPECT_EQ(res.errors()[0].category, "CONSTRAINT");
}

TEST_CASE(Invalid_07_InfiniteRHS) {
    LPModel model("inf_rhs");
    index_t x = model.add_variable("x", 0.0, 10.0);
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::LESS_EQUAL, BHARATOPT_INFINITY);
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
}

TEST_CASE(Invalid_08_InfiniteCoefficient) {
    LPModel model("inf_coeff");
    index_t x = model.add_variable("x", 0.0, 10.0);
    model.add_constraint("c1", {{x, BHARATOPT_INFINITY}}, ConstraintSense::LESS_EQUAL, 5.0);
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
}

TEST_CASE(Invalid_09_InvalidVariableIndexReference) {
    LPModel model("bad_var_ref");
    model.add_variable("x", 0.0, 10.0);
    // Bypass add_constraint checking to simulate malformed internal state / direct constraint modification
    Constraint cons;
    cons.name = "c1";
    cons.sense = ConstraintSense::LESS_EQUAL;
    cons.rhs = 5.0;
    cons.terms = {{99, 1.0}}; // Invalid var index 99
    
    // Add raw constraint
    try {
        model.add_constraint("c1", {{0, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);
        model.get_constraint(0).terms = {{99, 1.0}}; // Modify term directly to out of range
    } catch (...) {}

    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
    EXPECT_EQ(res.errors()[0].category, "COEFFICIENT");
}

TEST_CASE(Invalid_10_MalformedRangedConstraint) {
    LPModel model("bad_range");
    index_t x = model.add_variable("x", 0.0, 10.0);
    // Ranged constraint with lower range (rhs=10) > upper range (5)
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::RANGED, 10.0, 5.0);
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
    EXPECT_EQ(res.errors()[0].category, "CONSTRAINT");
}

TEST_CASE(Invalid_11_NaNRangeUpper) {
    LPModel model("nan_range");
    index_t x = model.add_variable("x", 0.0, 10.0);
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::RANGED, 2.0, std::numeric_limits<real_t>::quiet_NaN());
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
}

TEST_CASE(Invalid_12_InfiniteRangeUpper) {
    LPModel model("inf_range");
    index_t x = model.add_variable("x", 0.0, 10.0);
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::RANGED, 2.0, BHARATOPT_INFINITY);
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
}

TEST_CASE(Invalid_13_DuplicateVariableName) {
    LPModel model("dup_var");
    model.add_variable("x", 0.0, 10.0);
    try {
        model.add_variable("x", 1.0, 5.0);
    } catch (const std::invalid_argument&) {
        // Exception thrown at creation time
    }
    ModelValidator validator;
    // Model created without duplicate stays valid
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Invalid_14_DuplicateConstraintName) {
    LPModel model("dup_cons");
    index_t x = model.add_variable("x", 0.0, 10.0);
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);
    try {
        model.add_constraint("c1", {{x, 2.0}}, ConstraintSense::LESS_EQUAL, 10.0);
    } catch (const std::invalid_argument&) {
        // Handled cleanly at construction
    }
    ModelValidator validator;
    EXPECT_TRUE(validator.validate(model).is_valid());
}

TEST_CASE(Invalid_15_NaNObjectiveOffset) {
    LPModel model("nan_obj_off");
    model.set_obj_offset(std::numeric_limits<real_t>::quiet_NaN());
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
    EXPECT_EQ(res.errors()[0].category, "OBJECTIVE");
}

TEST_CASE(Invalid_16_InfiniteObjectiveOffset) {
    LPModel model("inf_obj_off");
    model.set_obj_offset(BHARATOPT_INFINITY);
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
    EXPECT_EQ(res.errors()[0].category, "OBJECTIVE");
}

TEST_CASE(Invalid_17_NaNObjectiveCoeff) {
    LPModel model("nan_obj_coeff");
    model.add_variable("x", 0.0, 10.0, std::numeric_limits<real_t>::quiet_NaN());
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
    EXPECT_EQ(res.errors()[0].category, "OBJECTIVE");
}

TEST_CASE(Invalid_18_InfiniteObjectiveCoeff) {
    LPModel model("inf_obj_coeff");
    model.add_variable("x", 0.0, 10.0, BHARATOPT_INFINITY);
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
    EXPECT_EQ(res.errors()[0].category, "OBJECTIVE");
}

TEST_CASE(Invalid_19_ZeroCoefficientWarning) {
    LPModel model("zero_coeff");
    index_t x = model.add_variable("x", 0.0, 10.0);
    model.add_constraint("c1", {{x, 0.0}}, ConstraintSense::LESS_EQUAL, 5.0);
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_TRUE(res.is_valid()); // Zero coeff is warning, not error
    EXPECT_TRUE(res.warning_count() > 0);
    EXPECT_EQ(res.warnings()[0].category, "COEFFICIENT");
}

TEST_CASE(Invalid_20_ImmutabilityAndDiagnosticsTest) {
    LPModel model("immutability_test");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 10.0, 5.0); // Invalid bounds (10 > 5)
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    // Record initial state
    size_t orig_num_vars = model.num_variables();
    size_t orig_num_cons = model.num_constraints();
    std::string orig_name = model.name();

    ModelValidator validator;
    ValidationResult res = validator.validate(model);

    // Verify validation detected error
    EXPECT_FALSE(res.is_valid());
    EXPECT_TRUE(res.error_count() >= 1);

    // Diagnostic check
    std::string diag_str = res.to_string();
    EXPECT_TRUE(diag_str.find("ERROR") != std::string::npos);
    EXPECT_TRUE(diag_str.find("x") != std::string::npos);

    // Immutability check: Model state unchanged
    EXPECT_EQ(model.num_variables(), orig_num_vars);
    EXPECT_EQ(model.num_constraints(), orig_num_cons);
    EXPECT_EQ(model.name(), orig_name);
}
