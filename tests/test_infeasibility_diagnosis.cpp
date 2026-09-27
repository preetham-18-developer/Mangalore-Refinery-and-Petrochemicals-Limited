#include <bharatopt/model_validator.hpp>
#include <bharatopt/mps_parser.hpp>
#include "test_harness.hpp"
#include <iostream>

namespace bharatopt {

TEST_CASE(InfeasibilityDiagnosisTest) {
    // 1. Test Infeasibility Diagnosis on share2b original infeasible model
    MpsParser parser;
    MpsParseResult parse_share2b = parser.parse_file("benchmarks/netlib/share2b.mps");
    
    // Create an artificial infeasible model if share2b was modified: C03 (X01>=4), C04 (X02=3), C01 (2X01+X02<=8)
    LPModel infeas_model("infeasible_share2b");
    index_t x1 = infeas_model.add_variable("X01", 0.0, 10.0, 3.0);
    index_t x2 = infeas_model.add_variable("X02", 0.0, 10.0, 5.0);

    // C01: 2*X01 + X02 <= 8.0
    infeas_model.add_constraint("C01", {{x1, 2.0}, {x2, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    // C03: X01 >= 4.0
    infeas_model.add_constraint("C03", {{x1, 1.0}}, ConstraintSense::GREATER_EQUAL, 4.0);
    // C04: X02 = 3.0
    infeas_model.add_constraint("C04", {{x2, 1.0}}, ConstraintSense::EQUAL, 3.0);

    InfeasibilityDiagnosis diag1 = InfeasibilityAnalyzer::analyze(infeas_model);
    std::cout << "\n=== INFEASIBILITY DIAGNOSIS TEST 1 ===\n";
    std::cout << "Is Infeasible: " << (diag1.is_infeasible ? "YES" : "NO") << "\n";
    std::cout << "IIS String   : " << diag1.to_string() << "\n";

    EXPECT_TRUE(diag1.is_infeasible);
    EXPECT_FALSE(diag1.conflicting_rows.empty());

    // 2. Test Procurement LP with RANGES infeasibility
    LPModel proc_model("infeasible_procurement");
    std::vector<index_t> s1_vars, s2_vars, s3_vars, s4_vars;
    std::vector<std::pair<index_t, real_t>> all_terms;

    for (int i = 1; i <= 5; ++i) {
        for (int j = 1; j <= 4; ++j) {
            index_t v = proc_model.add_variable("X" + std::to_string(i) + "_" + std::to_string(j), 0.0, 100.0, 10.0);
            all_terms.push_back({v, 1.0});
            if (j == 1) s1_vars.push_back(v);
            else if (j == 2) s2_vars.push_back(v);
            else if (j == 3) s3_vars.push_back(v);
            else if (j == 4) s4_vars.push_back(v);
        }
    }

    // SLOT1CAP: ranged [350, 400]
    std::vector<std::pair<index_t, real_t>> t1, t2, t3, t4;
    for (auto v : s1_vars) t1.push_back({v, 1.0});
    for (auto v : s2_vars) t2.push_back({v, 1.0});
    for (auto v : s3_vars) t3.push_back({v, 1.0});
    for (auto v : s4_vars) t4.push_back({v, 1.0});

    proc_model.add_constraint("SLOT1CAP", t1, ConstraintSense::RANGED, 350.0, 400.0);
    proc_model.add_constraint("SLOT2CAP", t2, ConstraintSense::RANGED, 310.0, 350.0);
    proc_model.add_constraint("SLOT3CAP", t3, ConstraintSense::RANGED, 270.0, 300.0);
    proc_model.add_constraint("SLOT4CAP", t4, ConstraintSense::RANGED, 220.0, 250.0);
    proc_model.add_constraint("TOTFARM", all_terms, ConstraintSense::EQUAL, 1000.0);

    InfeasibilityDiagnosis diag2 = InfeasibilityAnalyzer::analyze(proc_model);
    std::cout << "\n=== INFEASIBILITY DIAGNOSIS TEST 2 (PROCUREMENT) ===\n";
    std::cout << "Is Infeasible: " << (diag2.is_infeasible ? "YES" : "NO") << "\n";
    std::cout << "IIS String   : " << diag2.to_string() << "\n";

    EXPECT_TRUE(diag2.is_infeasible);
    EXPECT_TRUE(diag2.to_string().find("1150") != std::string::npos || diag2.to_string().find("forced min") != std::string::npos);
}

} // namespace bharatopt
