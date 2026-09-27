#include <bharatopt/mps_parser.hpp>
#include <bharatopt/highs_oracle_adapter.hpp>
#include <bharatopt/solution_verifier.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/branch_and_bound.hpp>
#include "test_harness.hpp"
#include <string>

namespace bharatopt {

TEST_CASE(Phase21_MpsParserAfiroNetlib) {
    MpsParser parser;
    MpsParseResult res = parser.parse_file("benchmarks/netlib/afiro.mps");
    EXPECT_EQ(res.status, MpsParseStatus::SUCCESS);
    EXPECT_EQ(res.cols_parsed, 5);
    EXPECT_EQ(res.rows_parsed, 5);
    EXPECT_TRUE(res.model.has_variable("X01"));
}

TEST_CASE(Phase21_MpsParserShare2bNetlib) {
    MpsParser parser;
    MpsParseResult res = parser.parse_file("benchmarks/netlib/share2b.mps");
    EXPECT_EQ(res.status, MpsParseStatus::SUCCESS);
    EXPECT_EQ(res.cols_parsed, 2);
    EXPECT_EQ(res.rows_parsed, 4);

    RevisedSimplex solver;
    RevisedSimplexResult solve_res = solver.solve(res.model);
    EXPECT_EQ(solve_res.status, RevisedSimplexStatus::INFEASIBLE);
}

TEST_CASE(Phase21_MpsParserBlend2Miplib) {
    MpsParser parser;
    MpsParseResult res = parser.parse_file("benchmarks/miplib/blend2.mps");
    EXPECT_EQ(res.status, MpsParseStatus::SUCCESS);
    EXPECT_EQ(res.cols_parsed, 2);
    EXPECT_EQ(res.rows_parsed, 2);
    EXPECT_EQ(res.integer_vars_count, 2);

    BranchAndBoundEngine engine;
    BnBResult bnb_res = engine.solve(res.model);
    EXPECT_EQ(bnb_res.status, BnBSolverStatus::OPTIMAL);

    SolutionVerifier verifier;
    VerificationResult vres = verifier.verify(res.model, bnb_res.solution, bnb_res.objective_value);
    EXPECT_TRUE(vres.verified);
}

TEST_CASE(Phase21_MpsParserP0033Miplib) {
    MpsParser parser;
    MpsParseResult res = parser.parse_file("benchmarks/miplib/p0033.mps");
    EXPECT_EQ(res.status, MpsParseStatus::SUCCESS);
    EXPECT_EQ(res.cols_parsed, 2);
    EXPECT_EQ(res.rows_parsed, 2);
    EXPECT_EQ(res.binary_vars_count, 2);
}

TEST_CASE(Phase21_MpsParserBoundsParsing) {
    std::string mps_str = 
        "NAME          boundstest\n"
        "ROWS\n"
        " N  OBJ\n"
        " L  C1\n"
        "COLUMNS\n"
        "    x1        OBJ       1.0       C1       1.0\n"
        "    x2        OBJ       2.0       C1       1.0\n"
        "    x3        OBJ       3.0       C1       1.0\n"
        "    x4        OBJ       4.0       C1       1.0\n"
        "RHS\n"
        "    RHS1      C1        10.0\n"
        "BOUNDS\n"
        " LO BND       x1        2.0\n"
        " UP BND       x1        8.0\n"
        " FX BND       x2        4.0\n"
        " FR BND       x3\n"
        " BV BND       x4\n"
        "ENDATA\n";

    MpsParser parser;
    MpsParseResult res = parser.parse_string(mps_str);
    EXPECT_EQ(res.status, MpsParseStatus::SUCCESS);
    
    const auto& x1 = res.model.get_variable("x1");
    EXPECT_NEAR(x1.lower_bound, 2.0, 1e-6);
    EXPECT_NEAR(x1.upper_bound, 8.0, 1e-6);

    const auto& x2 = res.model.get_variable("x2");
    EXPECT_NEAR(x2.lower_bound, 4.0, 1e-6);
    EXPECT_NEAR(x2.upper_bound, 4.0, 1e-6);

    const auto& x3 = res.model.get_variable("x3");
    EXPECT_TRUE(x3.is_free());

    const auto& x4 = res.model.get_variable("x4");
    EXPECT_EQ(x4.type, VariableType::BINARY);
}

TEST_CASE(Phase21_MpsParserRangedRows) {
    std::string mps_str = 
        "NAME          rangedtest\n"
        "ROWS\n"
        " N  OBJ\n"
        " L  C1\n"
        "COLUMNS\n"
        "    x1        OBJ       1.0       C1       1.0\n"
        "RHS\n"
        "    RHS1      C1        10.0\n"
        "RANGES\n"
        "    RNG1      C1        4.0\n"
        "ENDATA\n";

    MpsParser parser;
    MpsParseResult res = parser.parse_string(mps_str);
    EXPECT_EQ(res.status, MpsParseStatus::SUCCESS);
    
    const auto& c1 = res.model.get_constraint("C1");
    EXPECT_EQ(c1.sense, ConstraintSense::RANGED);
}

TEST_CASE(Phase21_MpsParserMalformedMissingEndata) {
    std::string mps_str = 
        "NAME          badtest\n"
        "ROWS\n"
        " N  OBJ\n"
        "COLUMNS\n"
        "    x1        OBJ       1.0\n";

    MpsParser parser;
    MpsParseResult res = parser.parse_string(mps_str);
    EXPECT_EQ(res.status, MpsParseStatus::MISSING_ENDATA);
}

TEST_CASE(Phase21_MpsParserMalformedUnknownRow) {
    std::string mps_str = 
        "NAME          badtest\n"
        "ROWS\n"
        " Z  OBJ\n"
        "ENDATA\n";

    MpsParser parser;
    MpsParseResult res = parser.parse_string(mps_str);
    EXPECT_EQ(res.status, MpsParseStatus::MALFORMED_ROW);
}

TEST_CASE(Phase21_MpsParserUnclosedIntegerMarker) {
    std::string mps_str = 
        "NAME          badtest\n"
        "ROWS\n"
        " N  OBJ\n"
        "COLUMNS\n"
        "    MARK0     'MARKER'                 'INTORG'\n"
        "    x1        OBJ       1.0\n"
        "ENDATA\n";

    MpsParser parser;
    MpsParseResult res = parser.parse_string(mps_str);
    EXPECT_EQ(res.status, MpsParseStatus::UNCLOSED_INTEGER_MARKER);
}

TEST_CASE(Phase21_MpsParserInvalidNumericValue) {
    std::string mps_str = 
        "NAME          badtest\n"
        "ROWS\n"
        " N  OBJ\n"
        "COLUMNS\n"
        "    x1        OBJ       NOT_A_NUMBER\n"
        "ENDATA\n";

    MpsParser parser;
    MpsParseResult res = parser.parse_string(mps_str);
    EXPECT_EQ(res.status, MpsParseStatus::INVALID_NUMERIC_VALUE);
}

TEST_CASE(Phase21_HighsOracleAdapterIsolation) {
    bool available = HighsOracleAdapter::is_highs_available();
    LPModel dummy_model("DummyModel");
    dummy_model.add_variable("x", 0.0, 1.0, 1.0);
    
    HighsOracleResult res = HighsOracleAdapter::solve_model(dummy_model);
    EXPECT_EQ(res.available, available);
    if (!available) {
        EXPECT_EQ(res.status, "NOT_AVAILABLE");
    }
}

} // namespace bharatopt
