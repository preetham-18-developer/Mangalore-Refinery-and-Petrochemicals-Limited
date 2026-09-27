#include <bharatopt/solution_verifier.hpp>
#include <bharatopt/educational_simplex.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/dual_revised_simplex.hpp>
#include <bharatopt/first_order_solver.hpp>
#include <bharatopt/branch_and_bound.hpp>
#include <bharatopt/presolve.hpp>
#include "test_harness.hpp"
#include <cmath>
#include <vector>

namespace bharatopt {

// ============================================================================
// A. BOUNDS TESTS
// ============================================================================

TEST_CASE(Phase20_ValidBounds) {
    LPModel model("ValidBoundsModel");
    model.add_variable("x", 0.0, 10.0, 1.0);
    model.add_variable("y", -5.0, 5.0, 2.0);

    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {5.0, 0.0});
    EXPECT_TRUE(res.verified);
    EXPECT_EQ(res.violated_bound_count, 0);
}

TEST_CASE(Phase20_LowerBoundViolation) {
    LPModel model("LBDViolationModel");
    model.add_variable("x", 2.0, 10.0, 1.0);

    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {1.0}); // x=1.0 < 2.0
    EXPECT_FALSE(res.verified);
    EXPECT_TRUE(res.violated_bound_count > 0);
    EXPECT_NEAR(res.maximum_lower_bound_violation, 1.0, 1e-6);
}

TEST_CASE(Phase20_UpperBoundViolation) {
    LPModel model("UBDViolationModel");
    model.add_variable("x", 0.0, 5.0, 1.0);

    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {6.5}); // x=6.5 > 5.0
    EXPECT_FALSE(res.verified);
    EXPECT_TRUE(res.violated_bound_count > 0);
    EXPECT_NEAR(res.maximum_upper_bound_violation, 1.5, 1e-6);
}

TEST_CASE(Phase20_FixedVariable) {
    LPModel model("FixedVarModel");
    model.add_variable("x", 3.0, 3.0, 1.0); // Fixed x = 3.0

    SolutionVerifier verifier;
    EXPECT_TRUE(verifier.verify(model, {3.0}).verified);
    EXPECT_FALSE(verifier.verify(model, {3.1}).verified);
}

TEST_CASE(Phase20_FreeVariable) {
    LPModel model("FreeVarModel");
    model.add_variable("x", -BHARATOPT_INFINITY, BHARATOPT_INFINITY, 1.0);

    SolutionVerifier verifier;
    EXPECT_TRUE(verifier.verify(model, {-1000.0}).verified);
    EXPECT_TRUE(verifier.verify(model, {1000.0}).verified);
}

// ============================================================================
// B. CONSTRAINTS TESTS
// ============================================================================

TEST_CASE(Phase20_ConstraintLessEqual) {
    LPModel model("LessEqualModel");
    index_t x = model.add_variable("x", 0.0, 10.0, 1.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{x, 1.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    SolutionVerifier verifier;
    EXPECT_TRUE(verifier.verify(model, {2.0, 3.0}).verified); // 2+3 = 5 <= 5
    EXPECT_FALSE(verifier.verify(model, {3.0, 3.0}).verified); // 3+3 = 6 > 5
}

TEST_CASE(Phase20_ConstraintGreaterEqual) {
    LPModel model("GreaterEqualModel");
    index_t x = model.add_variable("x", 0.0, 10.0, 1.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{x, 1.0}, {y, 2.0}}, ConstraintSense::GREATER_EQUAL, 6.0);

    SolutionVerifier verifier;
    EXPECT_TRUE(verifier.verify(model, {2.0, 2.0}).verified); // 2 + 4 = 6 >= 6
    EXPECT_FALSE(verifier.verify(model, {1.0, 2.0}).verified); // 1 + 4 = 5 < 6
}

TEST_CASE(Phase20_ConstraintEqual) {
    LPModel model("EqualModel");
    index_t x = model.add_variable("x", 0.0, 10.0, 1.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{x, 2.0}, {y, -1.0}}, ConstraintSense::EQUAL, 4.0);

    SolutionVerifier verifier;
    EXPECT_TRUE(verifier.verify(model, {3.0, 2.0}).verified); // 6 - 2 = 4
    EXPECT_FALSE(verifier.verify(model, {3.0, 1.0}).verified); // 6 - 1 = 5 != 4
}

TEST_CASE(Phase20_ConstraintRanged) {
    LPModel model("RangedModel");
    index_t x = model.add_variable("x", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::RANGED, 2.0, 8.0); // 2.0 <= x <= 8.0

    SolutionVerifier verifier;
    EXPECT_TRUE(verifier.verify(model, {5.0}).verified);
    EXPECT_FALSE(verifier.verify(model, {1.0}).verified); // 1.0 < 2.0
    EXPECT_FALSE(verifier.verify(model, {9.0}).verified); // 9.0 > 8.0
}

TEST_CASE(Phase20_MultipleConstraints) {
    LPModel model("MultiConsModel");
    index_t x = model.add_variable("x", 0.0, 10.0, 1.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{x, 1.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 2.0}, {y, 1.0}}, ConstraintSense::GREATER_EQUAL, 4.0);

    SolutionVerifier verifier;
    EXPECT_TRUE(verifier.verify(model, {3.0, 3.0}).verified);
    EXPECT_FALSE(verifier.verify(model, {0.0, 0.0}).verified); // Violates c2
}

TEST_CASE(Phase20_SparseMatrix) {
    LPModel model("SparseMatrixModel");
    std::vector<index_t> vars;
    for (int i = 0; i < 50; ++i) {
        vars.push_back(model.add_variable("x" + std::to_string(i), 0.0, 1.0, 1.0));
    }
    // Sparse constraint involving x[0] and x[49]
    model.add_constraint("c1", {{vars[0], 1.0}, {vars[49], 1.0}}, ConstraintSense::LESS_EQUAL, 1.5);

    SolutionVerifier verifier;
    std::vector<real_t> sol(50, 0.5);
    EXPECT_TRUE(verifier.verify(model, sol).verified);

    sol[0] = 1.0; sol[49] = 1.0; // 1 + 1 = 2 > 1.5
    EXPECT_FALSE(verifier.verify(model, sol).verified);
}

// ============================================================================
// C. OBJECTIVE TESTS
// ============================================================================

TEST_CASE(Phase20_MinimizationObjective) {
    LPModel model("MinObjModel");
    model.set_sense(ObjectiveSense::MINIMIZE);
    model.add_variable("x", 0.0, 10.0, 3.0);
    model.add_variable("y", 0.0, 10.0, 5.0);

    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {2.0, 4.0}, 26.0); // 3*2 + 5*4 = 26
    EXPECT_TRUE(res.verified);
    EXPECT_NEAR(res.objective_recomputed, 26.0, 1e-6);
    EXPECT_NEAR(res.objective_difference, 0.0, 1e-6);
}

TEST_CASE(Phase20_MaximizationObjective) {
    LPModel model("MaxObjModel");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    model.add_variable("x", 0.0, 10.0, 4.0);
    model.add_variable("y", 0.0, 10.0, 7.0);

    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {1.0, 2.0}, 18.0); // 4*1 + 7*2 = 18
    EXPECT_TRUE(res.verified);
    EXPECT_NEAR(res.objective_recomputed, 18.0, 1e-6);
}

TEST_CASE(Phase20_ObjectiveOffset) {
    LPModel model("OffsetObjModel");
    model.set_obj_offset(10.0);
    model.add_variable("x", 0.0, 10.0, 2.0);

    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {3.0}, 16.0); // 2*3 + 10 = 16
    EXPECT_TRUE(res.verified);
    EXPECT_NEAR(res.objective_recomputed, 16.0, 1e-6);
}

TEST_CASE(Phase20_ObjectiveMismatch) {
    LPModel model("MismatchObjModel");
    model.add_variable("x", 0.0, 10.0, 5.0);

    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {2.0}, 999.0); // True obj = 10, reported = 999
    EXPECT_FALSE(res.verified);
    EXPECT_NEAR(res.objective_recomputed, 10.0, 1e-6);
    EXPECT_TRUE(res.objective_difference > 10.0);
}

TEST_CASE(Phase20_ZeroObjective) {
    LPModel model("ZeroObjModel");
    model.add_variable("x", 0.0, 10.0, 0.0);

    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {5.0}, 0.0);
    EXPECT_TRUE(res.verified);
    EXPECT_NEAR(res.objective_recomputed, 0.0, 1e-6);
}

// ============================================================================
// D. INTEGRALITY TESTS
// ============================================================================

TEST_CASE(Phase20_ValidInteger) {
    LPModel model("ValidIntModel");
    model.add_variable("x", 0.0, 10.0, 1.0, VariableType::INTEGER);

    SolutionVerifier verifier;
    EXPECT_TRUE(verifier.verify(model, {4.0}).verified);
}

TEST_CASE(Phase20_FractionalInteger) {
    LPModel model("FracIntModel");
    model.add_variable("x", 0.0, 10.0, 1.0, VariableType::INTEGER);

    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {4.35});
    EXPECT_FALSE(res.verified);
    EXPECT_TRUE(res.violated_integrality_count > 0);
    EXPECT_NEAR(res.maximum_integrality_violation, 0.35, 1e-4);
}

TEST_CASE(Phase20_ValidBinaryZero) {
    LPModel model("ValidBin0Model");
    model.add_variable("x", 0.0, 1.0, 1.0, VariableType::BINARY);

    SolutionVerifier verifier;
    EXPECT_TRUE(verifier.verify(model, {0.0}).verified);
}

TEST_CASE(Phase20_ValidBinaryOne) {
    LPModel model("ValidBin1Model");
    model.add_variable("x", 0.0, 1.0, 1.0, VariableType::BINARY);

    SolutionVerifier verifier;
    EXPECT_TRUE(verifier.verify(model, {1.0}).verified);
}

TEST_CASE(Phase20_InvalidBinaryFractional) {
    LPModel model("FracBinModel");
    model.add_variable("x", 0.0, 1.0, 1.0, VariableType::BINARY);

    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {0.7});
    EXPECT_FALSE(res.verified);
    EXPECT_TRUE(res.violated_integrality_count > 0);
}

TEST_CASE(Phase20_InvalidBinaryOutOfRange) {
    LPModel model("OutOfRangeBinModel");
    model.add_variable("x", 0.0, 1.0, 1.0, VariableType::BINARY);

    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {2.0}); // Violates bound & integrality
    EXPECT_FALSE(res.verified);
}

// ============================================================================
// E. NUMERICAL & EDGE CASES TESTS
// ============================================================================

TEST_CASE(Phase20_NaNValue) {
    LPModel model("NaNModel");
    model.add_variable("x", 0.0, 10.0, 1.0);

    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {std::numeric_limits<real_t>::quiet_NaN()});
    EXPECT_FALSE(res.verified);
    EXPECT_TRUE(res.violated_bound_count > 0);
}

TEST_CASE(Phase20_InfValue) {
    LPModel model("InfModel");
    model.add_variable("x", 0.0, 10.0, 1.0);

    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {std::numeric_limits<real_t>::infinity()});
    EXPECT_FALSE(res.verified);
    EXPECT_TRUE(res.violated_bound_count > 0);
}

TEST_CASE(Phase20_SmallResidualTolerance) {
    LPModel model("SmallResidualModel");
    index_t x = model.add_variable("x", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    SolutionVerifier verifier;
    VerificationOptions opts;
    opts.feasibility_tolerance = 1e-4;
    // x = 5.00001 (within 1e-4 tolerance)
    EXPECT_TRUE(verifier.verify(model, {5.00001}, opts).verified);
    // x = 5.01 (exceeds 1e-4 tolerance)
    EXPECT_FALSE(verifier.verify(model, {5.01}, opts).verified);
}

TEST_CASE(Phase20_DimensionMismatch) {
    LPModel model("DimMismatchModel");
    model.add_variable("x", 0.0, 10.0, 1.0);
    model.add_variable("y", 0.0, 10.0, 1.0);

    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {1.0}); // 1 value for 2 vars
    EXPECT_FALSE(res.verified);
}

TEST_CASE(Phase20_ZeroVariableModel) {
    LPModel model("ZeroVarModel");
    SolutionVerifier verifier;
    EXPECT_TRUE(verifier.verify(model, {}).verified);
}

TEST_CASE(Phase20_ZeroConstraintModel) {
    LPModel model("ZeroConsModel");
    model.add_variable("x", 0.0, 5.0, 1.0);

    SolutionVerifier verifier;
    EXPECT_TRUE(verifier.verify(model, {2.0}).verified);
}

// ============================================================================
// F. INDEPENDENCE TESTS
// ============================================================================

TEST_CASE(Phase20_FakeOptimalStatusInfeasibleSol) {
    LPModel model("FakeOptimalInfeasible");
    index_t x = model.add_variable("x", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{x, 1.0}}, ConstraintSense::LESS_EQUAL, 2.0);

    // Candidate reports OPTIMAL status and objective 10.0, but solution x=5.0 is infeasible!
    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {5.0}, 10.0);
    EXPECT_FALSE(res.verified);
    EXPECT_TRUE(res.violated_constraint_count > 0);
}

TEST_CASE(Phase20_FakeObjective) {
    LPModel model("FakeObjectiveModel");
    model.add_variable("x", 0.0, 10.0, 3.0); // c = 3

    // Solution x=4 => True obj = 12, but candidate fake reports 50.0
    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {4.0}, 50.0);
    EXPECT_FALSE(res.verified);
    EXPECT_NEAR(res.objective_recomputed, 12.0, 1e-6);
}

TEST_CASE(Phase20_FakeIntegrality) {
    LPModel model("FakeIntegralityModel");
    model.add_variable("x", 0.0, 10.0, 1.0, VariableType::INTEGER);

    // Candidate solver says integer-feasible OPTIMAL, but x=3.4 is fractional!
    SolutionVerifier verifier;
    VerificationResult res = verifier.verify(model, {3.4});
    EXPECT_FALSE(res.verified);
    EXPECT_TRUE(res.violated_integrality_count > 0);
}

// ============================================================================
// G. SOLVER INTEGRATION TESTS
// ============================================================================

TEST_CASE(Phase20_EducationalSimplexIntegration) {
    LPModel model("EduSimplexVerifyModel");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 3.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 5.0);
    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    EducationalSimplex solver;
    SimplexResult res = solver.solve(model);
    EXPECT_EQ(res.status, SimplexStatus::OPTIMAL);

    SolutionVerifier verifier;
    VerificationResult vres = verifier.verify(model, res.primal_solution, res.objective_value);
    EXPECT_TRUE(vres.verified);
}

TEST_CASE(Phase20_RevisedSimplexIntegration) {
    LPModel model("RevisedSimplexVerifyModel");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 5.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 4.0);
    model.add_constraint("c1", {{x, 1.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    RevisedSimplex solver;
    RevisedSimplexResult res = solver.solve(model);
    EXPECT_EQ(res.status, RevisedSimplexStatus::OPTIMAL);

    SolutionVerifier verifier;
    VerificationResult vres = verifier.verify(model, res.primal_solution, res.objective_value);
    EXPECT_TRUE(vres.verified);
}

TEST_CASE(Phase20_DualRevisedSimplexIntegration) {
    LPModel model("DualSimplexVerifyModel");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 3.0);
    index_t y = model.add_variable("y", 0.0, 10.0, 5.0);
    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::GREATER_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::GREATER_EQUAL, 8.0);

    DualRevisedSimplex solver;
    DualRevisedSimplexResult res = solver.solve(model);
    EXPECT_EQ(res.status, DualRevisedSimplexStatus::OPTIMAL);

    SolutionVerifier verifier;
    VerificationResult vres = verifier.verify(model, res.primal_solution, res.objective_value);
    EXPECT_TRUE(vres.verified);
}

TEST_CASE(Phase20_FirstOrderSolverIntegration) {
    LPModel model("FirstOrderVerifyModel");
    model.set_sense(ObjectiveSense::MINIMIZE);
    index_t x = model.add_variable("x", 0.0, 2.0, 1.0);
    index_t y = model.add_variable("y", 0.0, 2.0, 2.0);
    model.add_constraint("c1", {{x, 1.0}, {y, 1.0}}, ConstraintSense::GREATER_EQUAL, 1.0);

    FirstOrderLPSolver solver;
    FirstOrderSolverResult res = solver.solve(model);
    EXPECT_EQ(res.status, FirstOrderSolverStatus::OPTIMAL);

    SolutionVerifier verifier;
    VerificationResult vres = verifier.verify(model, res.primal_solution, res.objective_value);
    EXPECT_TRUE(vres.verified);
}

TEST_CASE(Phase20_MilpCandidateIntegration) {
    LPModel model("MilpVerifyModel");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, 10.0, 3.0, VariableType::INTEGER);
    index_t y = model.add_variable("y", 0.0, 10.0, 5.0, VariableType::INTEGER);
    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    BranchAndBoundEngine engine;
    BnBResult res = engine.solve(model);
    EXPECT_EQ(res.status, BnBSolverStatus::OPTIMAL);

    SolutionVerifier verifier;
    VerificationResult vres = verifier.verify(model, res.solution, res.objective_value);
    EXPECT_TRUE(vres.verified);
    EXPECT_EQ(vres.violated_integrality_count, 0);
}

// ============================================================================
// H. PRESOLVE / POSTSOLVE VERIFICATION TEST
// ============================================================================

TEST_CASE(Phase20_PresolvePostsolveOriginalModelVerification) {
    LPModel original_model("OriginalPresolveModel");
    original_model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = original_model.add_variable("x", 0.0, 10.0, 3.0);
    index_t y = original_model.add_variable("y", 0.0, 10.0, 5.0);
    index_t z = original_model.add_variable("z", 4.0, 4.0, 2.0); // Fixed variable

    original_model.add_constraint("c1", {{x, 2.0}, {y, 1.0}, {z, 1.0}}, ConstraintSense::LESS_EQUAL, 12.0); // 2x + y + 4 <= 12 => 2x + y <= 8
    original_model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    PresolveEngine presolve;
    PresolveResult presolve_res = presolve.presolve(original_model);
    EXPECT_TRUE(presolve_res.reduced_model.num_variables() < original_model.num_variables());

    RevisedSimplex solver;
    RevisedSimplexResult solve_res = solver.solve(presolve_res.reduced_model);
    EXPECT_EQ(solve_res.status, RevisedSimplexStatus::OPTIMAL);

    // Map solution back to original model variable space via postsolve
    std::vector<real_t> orig_sol = presolve_res.postsolve.recover_solution(solve_res.primal_solution);
    EXPECT_EQ(orig_sol.size(), original_model.num_variables());

    // Verify mapped solution against ORIGINAL model (mandatory Phase 20 test)
    SolutionVerifier verifier;
    VerificationResult vres = verifier.verify(original_model, orig_sol);
    EXPECT_TRUE(vres.verified);
    EXPECT_NEAR(orig_sol[z], 4.0, 1e-6); // Fixed variable preserved
}

} // namespace bharatopt
