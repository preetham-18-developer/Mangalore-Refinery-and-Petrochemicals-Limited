#include "test_harness.hpp"
#include <bharatopt/execution_router.hpp>
#include <cstdio>

using namespace bharatopt;

TEST_CASE(Router_01_DeterministicDecisions) {
    BenchmarkInstanceConfig cfg;
    cfg.instance_id = "test_router_det";
    cfg.num_variables = 30;
    cfg.num_constraints = 15;
    cfg.seed = 101;

    LPModel model = BenchmarkGenerator::generate_instance(cfg);
    ExecutionRouter router;

    WorkloadFeatures feat = router.analyse(model);
    std::vector<CostEstimate> estimates = router.estimate(feat);

    RoutingDecision d1 = router.decide(model, feat, estimates);
    RoutingDecision d2 = router.decide(model, feat, estimates);

    EXPECT_EQ(d1.solver_name, d2.solver_name);
    EXPECT_EQ(d1.execution_device, d2.execution_device);
    EXPECT_EQ(d1.routing_reason, d2.routing_reason);
}

TEST_CASE(Router_02_ExplainableReasonGeneration) {
    LPModel model("explain_test");
    model.add_variable("x1", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{0, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    ExecutionRouter router;
    WorkloadFeatures feat = router.analyse(model);
    std::vector<CostEstimate> est = router.estimate(feat);
    RoutingDecision dec = router.decide(model, feat, est);

    EXPECT_FALSE(dec.routing_reason.empty());
    EXPECT_TRUE(dec.routing_reason.find("ms") != std::string::npos || dec.routing_reason.find("UNAVAILABLE") != std::string::npos || dec.routing_reason.find("ADAPTIVE") != std::string::npos);
}

TEST_CASE(Router_03_GpuUnavailableFallback) {
    LPModel model("gpu_unavail_test");
    model.add_variable("x1", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{0, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    RoutingConfiguration cfg;
    cfg.gpu_allowed = false; // Disallow GPU
    ExecutionRouter router(cfg);

    WorkloadFeatures feat = router.analyse(model);
    std::vector<CostEstimate> est = router.estimate(feat);
    RoutingDecision dec = router.decide(model, feat, est);

    EXPECT_EQ(dec.execution_device, "CPU");
    EXPECT_EQ(dec.solver_name, "DualRevisedSimplex");
    EXPECT_TRUE(dec.routing_reason.find("GPU_UNAVAILABLE") != std::string::npos);
}

TEST_CASE(Router_04_ExtrapolationWarningFallback) {
    LPModel model("extrap_test");

    RoutingConfiguration cfg;
    ExecutionRouter router(cfg);

    WorkloadFeatures out_domain_feat;
    out_domain_feat.m = 5000; // Out of calibration domain
    out_domain_feat.n = 10000;
    out_domain_feat.nnz = 100000;
    out_domain_feat.density = 0.50;
    out_domain_feat.gpu_available = true;
    out_domain_feat.cuda_available = true;

    std::vector<CostEstimate> est = router.estimate(out_domain_feat);
    RoutingDecision dec = router.decide(model, out_domain_feat, est);

    EXPECT_EQ(dec.execution_device, "CPU");
    EXPECT_EQ(dec.solver_name, "DualRevisedSimplex");
    EXPECT_EQ(dec.extrapolation_status, ExtrapolationStatus::EXTRAPOLATION_WARNING);
    EXPECT_TRUE(dec.routing_reason.find("EXTRAPOLATION_WARNING") != std::string::npos);
}

TEST_CASE(Router_05_ForcedRoutingModes) {
    LPModel model("forced_test");
    model.add_variable("x1", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{0, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    RoutingConfiguration cfg_rev;
    cfg_rev.routing_mode = RoutingMode::FORCE_CPU_REVISED;
    ExecutionRouter router_rev(cfg_rev);
    RoutedSolveResult res_rev = router_rev.solve(model);
    EXPECT_EQ(res_rev.decision.solver_name, "RevisedSimplex");

    RoutingConfiguration cfg_dual;
    cfg_dual.routing_mode = RoutingMode::FORCE_CPU_DUAL;
    ExecutionRouter router_dual(cfg_dual);
    RoutedSolveResult res_dual = router_dual.solve(model);
    EXPECT_EQ(res_dual.decision.solver_name, "DualRevisedSimplex");

    RoutingConfiguration cfg_fo;
    cfg_fo.routing_mode = RoutingMode::FORCE_CPU_FIRST_ORDER;
    ExecutionRouter router_fo(cfg_fo);
    RoutedSolveResult res_fo = router_fo.solve(model);
    EXPECT_EQ(res_fo.decision.solver_name, "FirstOrderLP");
    EXPECT_EQ(res_fo.decision.solver_variant, "CPU_PDHG");
}

TEST_CASE(Router_06_AutomaticGpuFailureFallback) {
    LPModel model("fallback_test");
    model.add_variable("x1", 0.0, 10.0, 1.0);
    model.add_constraint("c1", {{0, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    RoutingConfiguration cfg;
    cfg.routing_mode = RoutingMode::FORCE_GPU_FIRST_ORDER;
    cfg.fallback_enabled = true;

    ExecutionRouter router(cfg);
    RoutedSolveResult res = router.solve(model);

    EXPECT_EQ(res.status, "OPTIMAL");
    EXPECT_TRUE(res.verification_passed);
}

TEST_CASE(Router_07_HandDerivedLPRouting) {
    // Primary Hand-Derived Benchmark LP: Max 3x + 5y s.t. 2x + y <= 8, x + 2y <= 8
    // Optimum: x = 8/3, y = 8/3, obj = 64/3 (~21.333333)
    LPModel model("primary_hand_derived_router");
    model.set_sense(ObjectiveSense::MAXIMIZE);
    index_t x = model.add_variable("x", 0.0, BHARATOPT_INFINITY, 3.0);
    index_t y = model.add_variable("y", 0.0, BHARATOPT_INFINITY, 5.0);

    model.add_constraint("c1", {{x, 2.0}, {y, 1.0}}, ConstraintSense::LESS_EQUAL, 8.0);
    model.add_constraint("c2", {{x, 1.0}, {y, 2.0}}, ConstraintSense::LESS_EQUAL, 8.0);

    ExecutionRouter router;
    RoutedSolveResult res = router.solve(model);

    EXPECT_EQ(res.status, "OPTIMAL");
    EXPECT_TRUE(res.verification_passed);
    EXPECT_NEAR(res.primal_solution[x], 8.0 / 3.0, 1e-2);
    EXPECT_NEAR(res.primal_solution[y], 8.0 / 3.0, 1e-2);
    EXPECT_NEAR(res.objective_value, 64.0 / 3.0, 1e-2);
}

TEST_CASE(Router_08_UnsupportedModelRejection) {
    LPModel model("milp_unsupported_test");
    model.add_variable("x_int", 0.0, 10.0, 1.0, VariableType::INTEGER); // Integer variable
    model.add_constraint("c1", {{0, 1.0}}, ConstraintSense::LESS_EQUAL, 5.0);

    ExecutionRouter router;
    WorkloadFeatures feat = router.analyse(model);
    std::vector<CostEstimate> est = router.estimate(feat);
    RoutingDecision dec = router.decide(model, feat, est);

    EXPECT_EQ(dec.execution_device, "CPU");
    EXPECT_TRUE(dec.routing_reason.find("UNSUPPORTED_MODEL") != std::string::npos);
}

TEST_CASE(Router_09_RoutingLogSerialization) {
    RoutingEvaluationRecord rec;
    rec.instance_id = "test_routing_inst";
    rec.selected_solver = "DualRevisedSimplex-CPU_Dual_Simplex";
    rec.selected_device = "CPU";
    rec.predicted_cpu_cost_ms = 8.5;
    rec.predicted_gpu_cost_ms = 27.5;
    rec.actual_solve_time_ms = 8.37;
    rec.actual_best_eligible_time_ms = 8.37;
    rec.regret_ms = 0.0;
    rec.optimal_path_selected = true;
    rec.routing_reason = "ADAPTIVE_CPU_SELECTED";
    rec.verification_passed = true;

    std::vector<RoutingEvaluationRecord> records = {rec};
    std::string csv_path = "temp_routing_results.csv";
    std::string json_path = "temp_routing_results.json";

    EXPECT_TRUE(RouterEvaluator::export_routing_report_csv(records, csv_path));
    EXPECT_TRUE(RouterEvaluator::export_routing_report_json(records, json_path));

    std::remove(csv_path.c_str());
    std::remove(json_path.c_str());
}
