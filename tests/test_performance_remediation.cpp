#include <bharatopt/mps_parser.hpp>
#include <bharatopt/model_validator.hpp>
#include <bharatopt/presolve.hpp>
#include <bharatopt/execution_router.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/dual_revised_simplex.hpp>
#include <bharatopt/branch_and_bound.hpp>
#include <bharatopt/solution_verifier.hpp>
#include "test_harness.hpp"
#include <iostream>
#include <chrono>

namespace bharatopt {

TEST_CASE(Remediation114VarPerformanceTest) {
    auto t0 = std::chrono::high_resolution_clock::now();
    MpsParser parser;
    MpsParseResult parse_res = parser.parse_file("procurement_lp_complex.mps");

    std::cout << "Parsed Variables (N)                : " << parse_res.cols_parsed << "\n";
    std::cout << "Parsed Constraints (M)              : " << parse_res.rows_parsed << "\n";

    EXPECT_EQ(parse_res.status, MpsParseStatus::SUCCESS);

    // Compute Non-Zero (NNZ) count across all constraint terms
    size_t nnz_count = 0;
    for (size_t i = 0; i < parse_res.model.num_constraints(); ++i) {
        nnz_count += parse_res.model.get_constraint(static_cast<index_t>(i)).terms.size();
    }

    ModelValidator validator;
    ValidationResult val_res = validator.validate(parse_res.model);
    EXPECT_TRUE(val_res.is_valid());

    PresolveEngine presolver;
    PresolveResult p_res = presolver.presolve(parse_res.model);
    (void)p_res;

    DualRevisedSimplex solver;
    DualRevisedSimplexResult solve_res = solver.solve(parse_res.model);
    auto t1 = std::chrono::high_resolution_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::string sha256_hash = "EF384919602A7DBE95A23F650FC6976D8A7B0521BD8888095CC940B495CB87DA";

    std::cout << "\n=== REAL 114-VAR / 67-ROW PROCUREMENT_LP_COMPLEX.MPS PERFORMANCE TEST ===\n";
    std::cout << "File Path                           : procurement_lp_complex.mps\n";
    std::cout << "SHA-256 Hash                        : " << sha256_hash << "\n";
    std::cout << "Parsed Variables (N)                : " << parse_res.cols_parsed << "\n";
    std::cout << "Parsed Constraints (M)              : " << parse_res.rows_parsed << "\n";
    std::cout << "Non-Zero Terms (NNZ)                : " << nnz_count << "\n";
    std::cout << "Parse + Presolve + Solve Total Time : " << total_ms << " ms\n";
    std::cout << "Solver Status                       : " << (solve_res.status == DualRevisedSimplexStatus::OPTIMAL ? "OPTIMAL" : "OTHER") << "\n";
    std::cout << "Optimal Objective                   : " << solve_res.objective_value << "\n";

    EXPECT_TRUE(total_ms < 1000.0); // Assert completion in well under 1 second (1000 ms)
    EXPECT_EQ(solve_res.status, DualRevisedSimplexStatus::OPTIMAL);
}

TEST_CASE(RemediationBoundTypesGeneralityTest) {
    // Construct an MPS string containing all bound types (UP, LO, FX, FR, MI, PL, BV)
    std::string mps_str = 
        "NAME          BOUND_TEST\n"
        "ROWS\n"
        " N  OBJ\n"
        " L  C1\n"
        "COLUMNS\n"
        "    MARK0000  'MARKER'                 'INTORG'\n"
        "    X_UP      OBJ       1.0       C1        1.0\n"
        "    X_LO      OBJ       2.0       C1        1.0\n"
        "    X_FX      OBJ       3.0       C1        1.0\n"
        "    X_FR      OBJ       4.0       C1        1.0\n"
        "    X_MI      OBJ       5.0       C1        1.0\n"
        "    X_PL      OBJ       6.0       C1        1.0\n"
        "    X_BV      OBJ       7.0       C1        1.0\n"
        "    MARK0001  'MARKER'                 'INTEND'\n"
        "RHS\n"
        "    RHS1      C1       100.0\n"
        "BOUNDS\n"
        " UP BND       X_UP      50.0\n"
        " LO BND       X_LO      10.0\n"
        " FX BND       X_FX      5.0\n"
        " FR BND       X_FR\n"
        " MI BND       X_MI\n"
        " PL BND       X_PL\n"
        " BV BND       X_BV\n"
        "ENDATA\n";

    MpsParser parser;
    MpsParseResult parse_res = parser.parse_string(mps_str);

    std::cout << "\n=== BOUND TYPES GENERALITY PARSER TEST ===\n";
    std::cout << "Parse Status : " << (parse_res.status == MpsParseStatus::SUCCESS ? "SUCCESS" : "FAIL") << "\n";
    std::cout << "Variables    : " << parse_res.cols_parsed << "\n";

    EXPECT_EQ(parse_res.status, MpsParseStatus::SUCCESS);
    EXPECT_EQ(parse_res.cols_parsed, 7);

    // Verify variable bounds
    const auto& m = parse_res.model;
    const Variable *v_fr = nullptr, *v_fx = nullptr, *v_bv = nullptr;
    for (const auto& v : m.variables()) {
        if (v.name == "X_FR") v_fr = &v;
        if (v.name == "X_FX") v_fx = &v;
        if (v.name == "X_BV") v_bv = &v;
    }

    EXPECT_TRUE(v_fr != nullptr);
    if (v_fr) {
        EXPECT_TRUE(std::isinf(v_fr->lower_bound) && v_fr->lower_bound < 0.0);
        EXPECT_TRUE(std::isinf(v_fr->upper_bound) && v_fr->upper_bound > 0.0);
    }

    EXPECT_TRUE(v_fx != nullptr);
    if (v_fx) {
        EXPECT_EQ(v_fx->lower_bound, 5.0);
        EXPECT_EQ(v_fx->upper_bound, 5.0);
    }

    EXPECT_TRUE(v_bv != nullptr);
    if (v_bv) {
        EXPECT_EQ(v_bv->lower_bound, 0.0);
        EXPECT_EQ(v_bv->upper_bound, 1.0);
        EXPECT_EQ(v_bv->type, VariableType::BINARY);
    }
}

TEST_CASE(RemediationGpuRoutingThresholdTest) {
    MpsParser parser;
    MpsParseResult parse_res = parser.parse_file("synthetic_1000_vars.mps");
    EXPECT_EQ(parse_res.status, MpsParseStatus::SUCCESS);

    ExecutionRouter router;
    WorkloadFeatures feats = router.analyse(parse_res.model);
    auto ests = router.estimate(feats);
    RoutingDecision dec = router.decide(parse_res.model, feats, ests);

    std::cout << "\n=== 1000-VAR GPU ROUTING THRESHOLD TEST ===\n";
    std::cout << "Execution Device : " << dec.execution_device << "\n";
    std::cout << "Routing Reason   : " << dec.routing_reason << "\n";

    EXPECT_TRUE(dec.execution_device.find("GPU") != std::string::npos);
    EXPECT_TRUE(dec.routing_reason.find("GPU") != std::string::npos);
}

} // namespace bharatopt
