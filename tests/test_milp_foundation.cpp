#include "test_harness.hpp"
#include <bharatopt/milp_foundation.hpp>
#include <cmath>

using namespace bharatopt;

// ========================================================
// CATEGORY A: MODEL CLASSIFICATION TESTS
// ========================================================

TEST_CASE(MilpClassify_01_PureLP) {
    LPModel model("pure_lp");
    model.add_variable("x1", 0.0, 10.0, 1.0, VariableType::CONTINUOUS);
    model.add_variable("x2", 0.0, 10.0, 2.0, VariableType::CONTINUOUS);
    
    ModelType type = MilpFoundation::classify_model(model);
    EXPECT_EQ(type, ModelType::LP);
    EXPECT_EQ(model_type_to_string(type), "LP");
}

TEST_CASE(MilpClassify_02_OneIntegerVariable) {
    LPModel model("one_int");
    model.add_variable("x1", 0.0, 10.0, 1.0, VariableType::CONTINUOUS);
    model.add_variable("y1", 0.0, 5.0, 2.0, VariableType::INTEGER);
    
    ModelType type = MilpFoundation::classify_model(model);
    EXPECT_EQ(type, ModelType::MILP);
    EXPECT_EQ(model_type_to_string(type), "MILP");
}

TEST_CASE(MilpClassify_03_MultipleIntegerVariables) {
    LPModel model("multi_int");
    model.add_variable("y1", 0.0, 5.0, 1.0, VariableType::INTEGER);
    model.add_variable("y2", 0.0, 5.0, 2.0, VariableType::INTEGER);
    
    ModelType type = MilpFoundation::classify_model(model);
    EXPECT_EQ(type, ModelType::MILP);
}

TEST_CASE(MilpClassify_04_OneBinaryVariable) {
    LPModel model("one_bin");
    model.add_variable("x1", 0.0, 10.0, 1.0, VariableType::CONTINUOUS);
    model.add_variable("b1", 0.0, 1.0, 3.0, VariableType::BINARY);
    
    ModelType type = MilpFoundation::classify_model(model);
    EXPECT_EQ(type, ModelType::MILP);
}

TEST_CASE(MilpClassify_05_MixedVariables) {
    LPModel model("mixed");
    model.add_variable("x1", 0.0, 10.0, 1.0, VariableType::CONTINUOUS);
    model.add_variable("y1", 0.0, 10.0, 2.0, VariableType::INTEGER);
    model.add_variable("b1", 0.0, 1.0, 3.0, VariableType::BINARY);
    
    ModelType type = MilpFoundation::classify_model(model);
    EXPECT_EQ(type, ModelType::MILP);
}

// ========================================================
// CATEGORY B: BINARY BOUND VALIDATION TESTS
// ========================================================

TEST_CASE(MilpValid_01_ValidBinaryBounds) {
    LPModel model("valid_bin");
    model.add_variable("b1", 0.0, 1.0, 1.0, VariableType::BINARY);
    
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_TRUE(res.is_valid());
}

TEST_CASE(MilpValid_02_NegativeBinaryLowerBound) {
    LPModel model("invalid_bin_neg");
    model.add_variable("b1", -1.0, 1.0, 1.0, VariableType::BINARY);
    
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
}

TEST_CASE(MilpValid_03_BinaryUpperBoundExceeded) {
    LPModel model("invalid_bin_high");
    model.add_variable("b1", 0.0, 2.0, 1.0, VariableType::BINARY);
    
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
}

TEST_CASE(MilpValid_04_FixedBinaryVariable) {
    LPModel model("fixed_bin");
    model.add_variable("b1", 1.0, 1.0, 1.0, VariableType::BINARY);
    
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_TRUE(res.is_valid());
}

// ========================================================
// CATEGORY C: INTEGER BOUND VALIDATION TESTS
// ========================================================

TEST_CASE(MilpValid_05_ValidIntegerBounds) {
    LPModel model("valid_int");
    model.add_variable("y1", 0.0, 10.0, 1.0, VariableType::INTEGER);
    
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_TRUE(res.is_valid());
}

TEST_CASE(MilpValid_06_NegativeIntegerBounds) {
    LPModel model("neg_int");
    model.add_variable("y1", -5.0, 5.0, 1.0, VariableType::INTEGER);
    
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_TRUE(res.is_valid());
}

TEST_CASE(MilpValid_07_InvalidIntegerBoundsOrder) {
    LPModel model("invalid_int_order");
    model.add_variable("y1", 5.0, 2.0, 1.0, VariableType::INTEGER);
    
    ModelValidator validator;
    ValidationResult res = validator.validate(model);
    EXPECT_FALSE(res.is_valid());
}

// ========================================================
// CATEGORY D: LP RELAXATION EXTRACTION TESTS
// ========================================================

TEST_CASE(MilpRelax_01_PreservationOfModelStructure) {
    LPModel model("milp_struct");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    model.set_obj_offset(15.5);
    
    index_t x_idx = model.add_variable("x", 0.0, 10.0, 3.0, VariableType::CONTINUOUS);
    index_t y_idx = model.add_variable("y", 0.0, 8.0, 5.0, VariableType::INTEGER);
    index_t b_idx = model.add_variable("b", 0.0, 1.0, 2.0, VariableType::BINARY);
    
    model.add_constraint("c1", {{x_idx, 2.0}, {y_idx, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x_idx, 1.0}, {b_idx, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    
    LPRelaxation rel = MilpFoundation::extract_relaxation(model);
    
    EXPECT_EQ(rel.relaxation_model.sense(), ObjectiveSense::MAXIMIZE);
    EXPECT_NEAR(rel.relaxation_model.obj_offset(), 15.5, 1e-9);
    EXPECT_EQ(rel.relaxation_model.num_variables(), static_cast<size_t>(3));
    EXPECT_EQ(rel.relaxation_model.num_constraints(), static_cast<size_t>(2));
    
    for (size_t j = 0; j < rel.relaxation_model.num_variables(); ++j) {
        const auto& v = rel.relaxation_model.get_variable(static_cast<index_t>(j));
        EXPECT_EQ(v.type, VariableType::CONTINUOUS);
    }
    
    EXPECT_NEAR(rel.relaxation_model.get_variable("y").lower_bound, 0.0, 1e-9);
    EXPECT_NEAR(rel.relaxation_model.get_variable("y").upper_bound, 8.0, 1e-9);
    EXPECT_NEAR(rel.relaxation_model.get_variable("b").lower_bound, 0.0, 1e-9);
    EXPECT_NEAR(rel.relaxation_model.get_variable("b").upper_bound, 1.0, 1e-9);
}

// ========================================================
// CATEGORY E: IMMUTABILITY TESTS
// ========================================================

TEST_CASE(MilpImmutability_01_OriginalModelUnchanged) {
    LPModel original("orig_milp");
    index_t y_idx = original.add_variable("y", 0.0, 10.0, 5.0, VariableType::INTEGER);
    original.add_constraint("c1", {{y_idx, 1.0}}, ConstraintSense::LESS_EQUAL, 7.0);
    
    LPRelaxation rel = MilpFoundation::extract_relaxation(original);
    
    // Modify relaxation model
    rel.relaxation_model.get_variable(y_idx).upper_bound = 100.0;
    rel.relaxation_model.set_obj_offset(99.0);
    
    // Assert original remains completely unchanged
    EXPECT_EQ(original.get_variable(y_idx).type, VariableType::INTEGER);
    EXPECT_NEAR(original.get_variable(y_idx).upper_bound, 10.0, 1e-9);
    EXPECT_NEAR(original.obj_offset(), 0.0, 1e-9);
}

// ========================================================
// CATEGORY F: INTEGER FEASIBILITY CHECKER TESTS
// ========================================================

TEST_CASE(MilpFeas_01_IntegerSolutionPassed) {
    LPModel model("feas_test");
    model.add_variable("x", 0.0, 10.0, 1.0, VariableType::CONTINUOUS);
    model.add_variable("y", 0.0, 10.0, 1.0, VariableType::INTEGER);
    model.add_variable("b", 0.0, 1.0, 1.0, VariableType::BINARY);
    
    std::vector<real_t> solution = {2.5, 4.0, 1.0};
    
    IntegerVerificationResult res = MilpFoundation::verify_integer_feasibility(model, solution);
    EXPECT_TRUE(res.is_integer_feasible);
    EXPECT_EQ(res.violating_variable_count, static_cast<size_t>(0));
    EXPECT_NEAR(res.max_integrality_violation, 0.0, 1e-9);
}

TEST_CASE(MilpFeas_02_FractionalIntegerRejected) {
    LPModel model("frac_test");
    model.add_variable("x", 0.0, 10.0, 1.0, VariableType::CONTINUOUS);
    model.add_variable("y", 0.0, 10.0, 1.0, VariableType::INTEGER);
    
    std::vector<real_t> solution = {2.5, 4.33333};
    
    IntegerVerificationResult res = MilpFoundation::verify_integer_feasibility(model, solution);
    EXPECT_FALSE(res.is_integer_feasible);
    EXPECT_EQ(res.violating_variable_count, static_cast<size_t>(1));
    EXPECT_NEAR(res.max_integrality_violation, 0.33333, 1e-3);
}

TEST_CASE(MilpFeas_03_FractionalBinaryRejected) {
    LPModel model("frac_bin_test");
    model.add_variable("b", 0.0, 1.0, 1.0, VariableType::BINARY);
    
    std::vector<real_t> solution = {0.5};
    
    IntegerVerificationResult res = MilpFoundation::verify_integer_feasibility(model, solution);
    EXPECT_FALSE(res.is_integer_feasible);
    EXPECT_NEAR(res.max_integrality_violation, 0.5, 1e-9);
}

// ========================================================
// CATEGORY H: MANDATORY HAND-DERIVED MILP CASES
// ========================================================

TEST_CASE(MilpHand_01_HandDerivedLPRelaxationVsMilp) {
    // Hand-derived problem:
    // Maximize 3x + 5y
    // Subject to:
    // 2x + y <= 8
    // x + 2y <= 8
    // x, y >= 0, integer
    
    LPModel milp("hand_milp_1");
    milp.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x_idx = milp.add_variable("x", 0.0, 10.0, 3.0, VariableType::INTEGER);
    index_t y_idx = milp.add_variable("y", 0.0, 10.0, 5.0, VariableType::INTEGER);
    
    milp.add_constraint("c1", {{x_idx, 2.0}, {y_idx, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    milp.add_constraint("c2", {{x_idx, 1.0}, {y_idx, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    
    MilpRelaxationResult res = MilpFoundation::solve_relaxation(milp);
    
    EXPECT_EQ(res.status, MilpSolutionStatus::LP_RELAXATION_OPTIMAL);
    EXPECT_EQ(res.model_type, ModelType::MILP);
    EXPECT_NEAR(res.objective_value, 21.333333, 1e-4);
    EXPECT_NEAR(res.primal_solution[0], 2.666667, 1e-4);
    EXPECT_NEAR(res.primal_solution[1], 2.666667, 1e-4);
    EXPECT_FALSE(res.integer_verification.is_integer_feasible);
    EXPECT_EQ(res.integer_verification.violating_variable_count, static_cast<size_t>(2));
}

TEST_CASE(MilpHand_02_BinaryConstraintRelaxation) {
    // Maximize x + y
    // Subject to:
    // x + y <= 1
    // x, y binary
    
    LPModel milp("hand_milp_2");
    milp.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x_idx = milp.add_variable("x", 0.0, 1.0, 1.0, VariableType::BINARY);
    index_t y_idx = milp.add_variable("y", 0.0, 1.0, 1.0, VariableType::BINARY);
    
    milp.add_constraint("c1", {{x_idx, 1.0}, {y_idx, 1.0}}, ConstraintSense::LESS_EQUAL, 1.0);
    
    MilpRelaxationResult res = MilpFoundation::solve_relaxation(milp);
    
    EXPECT_NEAR(res.objective_value, 1.0, 1e-4);
    // Objective is 1.0; whether LP solver selects (1,0), (0,1), or (0.5,0.5), verification works correctly
    if (res.integer_verification.is_integer_feasible) {
        EXPECT_EQ(res.status, MilpSolutionStatus::INTEGER_FEASIBLE_SOLUTION);
    } else {
        EXPECT_EQ(res.status, MilpSolutionStatus::LP_RELAXATION_OPTIMAL);
    }
}

TEST_CASE(MilpHand_03_NaturallyIntegerFeasibleRelaxation) {
    // Minimize x + 2y
    // Subject to:
    // x + y >= 3
    // x, y >= 0, integer
    
    LPModel milp("hand_milp_3");
    milp.set_sense(ObjectiveSense::MINIMIZE);
    index_t x_idx = milp.add_variable("x", 0.0, 10.0, 1.0, VariableType::INTEGER);
    index_t y_idx = milp.add_variable("y", 0.0, 10.0, 2.0, VariableType::INTEGER);
    
    milp.add_constraint("c1", {{x_idx, 1.0}, {y_idx, 1.0}}, ConstraintSense::GREATER_EQUAL, 3.0);
    
    MilpRelaxationResult res = MilpFoundation::solve_relaxation(milp);
    
    EXPECT_EQ(res.status, MilpSolutionStatus::INTEGER_FEASIBLE_SOLUTION);
    EXPECT_NEAR(res.objective_value, 3.0, 1e-4);
    EXPECT_NEAR(res.primal_solution[0], 3.0, 1e-4);
    EXPECT_NEAR(res.primal_solution[1], 0.0, 1e-4);
    EXPECT_TRUE(res.integer_verification.is_integer_feasible);
}
