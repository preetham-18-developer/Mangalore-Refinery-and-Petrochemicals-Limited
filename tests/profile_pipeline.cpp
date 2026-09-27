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
#include <iomanip>
#include <chrono>
#include <fstream>

namespace bharatopt {

struct PipelineStageTimings {
    std::string model_name;
    size_t vars{0};
    size_t rows{0};
    size_t nnz{0};
    double parse_time_ms{0.0};
    double validate_time_ms{0.0};
    double presolve_time_ms{0.0};
    double router_time_ms{0.0};
    double solve_time_ms{0.0};
    double verify_time_ms{0.0};
    double total_time_ms{0.0};
    std::string solver_used;
    std::string final_status;
};

static PipelineStageTimings profile_model(const std::string& filepath) {
    PipelineStageTimings t;
    t.model_name = filepath;

    auto t_start_total = std::chrono::high_resolution_clock::now();

    // 1. Parse
    auto t0 = std::chrono::high_resolution_clock::now();
    MpsParser parser;
    MpsParseResult parse_res = parser.parse_file(filepath);
    auto t1 = std::chrono::high_resolution_clock::now();
    t.parse_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (parse_res.status != MpsParseStatus::SUCCESS) {
        t.final_status = "PARSE_ERROR";
        return t;
    }

    t.vars = parse_res.model.num_variables();
    t.rows = parse_res.model.num_constraints();
    for (const auto& c : parse_res.model.constraints()) t.nnz += c.terms.size();

    // 2. Validate & IIS Analysis
    t0 = std::chrono::high_resolution_clock::now();
    ModelValidator validator;
    ValidationResult val_res = validator.validate(parse_res.model);
    InfeasibilityDiagnosis diag = InfeasibilityAnalyzer::analyze(parse_res.model);
    t1 = std::chrono::high_resolution_clock::now();
    t.validate_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // 3. Presolve
    t0 = std::chrono::high_resolution_clock::now();
    PresolveEngine presolver;
    PresolveResult p_res = presolver.presolve(parse_res.model);
    t1 = std::chrono::high_resolution_clock::now();
    t.presolve_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    LPModel active_model = (p_res.status == PresolveStatus::SUCCESS) ? p_res.reduced_model : parse_res.model;

    // 4. Execution Router
    t0 = std::chrono::high_resolution_clock::now();
    ExecutionRouter router;
    WorkloadFeatures feats = router.analyse(active_model);
    auto ests = router.estimate(feats);
    RoutingDecision decision = router.decide(active_model, feats, ests);
    t1 = std::chrono::high_resolution_clock::now();
    t.router_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    t.solver_used = decision.solver_name;

    // 5. Solve
    t0 = std::chrono::high_resolution_clock::now();
    bool is_milp = false;
    for (const auto& v : active_model.variables()) {
        if (v.type == VariableType::INTEGER || v.type == VariableType::BINARY) { is_milp = true; break; }
    }

    std::vector<real_t> solution;
    real_t obj = 0.0;
    if (is_milp) {
        BnBConfig bnb_cfg;
        bnb_cfg.time_limit_ms = 5000.0; // 5 second budget for profiling
        BranchAndBoundEngine bnb(bnb_cfg);
        BnBResult b_res = bnb.solve(active_model);
        solution = b_res.solution;
        obj = b_res.objective_value;
        if (b_res.status == BnBSolverStatus::OPTIMAL) t.final_status = "OPTIMAL";
        else if (b_res.status == BnBSolverStatus::INFEASIBLE) t.final_status = "INFEASIBLE";
        else t.final_status = "TIME_LIMIT_REACHED";
    } else {
        DualRevisedSimplexOptions d_opts;
        d_opts.max_iterations = 5000;
        DualRevisedSimplex solver(d_opts);
        DualRevisedSimplexResult d_res = solver.solve(active_model);
        solution = d_res.primal_solution;
        obj = d_res.objective_value;
        if (d_res.status == DualRevisedSimplexStatus::OPTIMAL) t.final_status = "OPTIMAL";
        else if (d_res.status == DualRevisedSimplexStatus::INFEASIBLE) t.final_status = "INFEASIBLE";
        else if (d_res.status == DualRevisedSimplexStatus::UNBOUNDED) t.final_status = "UNBOUNDED";
        else t.final_status = "TIME_LIMIT_REACHED";
    }
    t1 = std::chrono::high_resolution_clock::now();
    t.solve_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // 6. Verify
    t0 = std::chrono::high_resolution_clock::now();
    if (t.final_status == "OPTIMAL" && !solution.empty()) {
        SolutionVerifier verifier;
        std::vector<real_t> full_sol = solution;
        if (p_res.status == PresolveStatus::SUCCESS) {
            full_sol = p_res.postsolve.recover_solution(solution);
        }
        VerificationResult v_res = verifier.verify(parse_res.model, full_sol, obj);
        (void)v_res;
    }
    t1 = std::chrono::high_resolution_clock::now();
    t.verify_time_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    auto t_end_total = std::chrono::high_resolution_clock::now();
    t.total_time_ms = std::chrono::duration<double, std::milli>(t_end_total - t_start_total).count();

    return t;
}

TEST_CASE(PipelineStageProfilingTest) {
    std::cout << "\n=========================================================================================================\n";
    std::cout << "                                BHARATOPT PIPELINE STAGE PROFILING TABLE                                  \n";
    std::cout << "=========================================================================================================\n";
    std::cout << std::left 
              << std::setw(28) << "Model Path"
              << std::setw(8)  << "Vars(N)"
              << std::setw(8)  << "Rows(M)"
              << std::setw(8)  << "NNZ"
              << std::setw(10) << "Parse(ms)"
              << std::setw(10) << "Valid(ms)"
              << std::setw(10) << "Presol(ms)"
              << std::setw(10) << "Solve(ms)"
              << std::setw(10) << "Total(ms)"
              << std::setw(16) << "Status"
              << "\n";
    std::cout << "---------------------------------------------------------------------------------------------------------\n";

    std::vector<std::string> test_models = {
        "example1.mps",
        "procurement_lp_test.mps",
        "procurement_lp_complex.mps",
        "benchmarks/netlib/afiro.mps",
        "benchmarks/netlib/share2b.mps",
        "benchmarks/miplib/p0033.mps",
        "benchmarks/miplib/blend2.mps",
        "bharatopt_refinery_demo.mps",
        "synthetic_114_vars.mps",
        "synthetic_300_vars.mps",
        "synthetic_1000_vars.mps"
    };

    for (const auto& path : test_models) {
        PipelineStageTimings t = profile_model(path);
        std::cout << std::left 
                  << std::setw(28) << (path.length() > 27 ? path.substr(0, 24) + "..." : path)
                  << std::setw(8)  << t.vars
                  << std::setw(8)  << t.rows
                  << std::setw(8)  << t.nnz
                  << std::fixed << std::setprecision(2)
                  << std::setw(10) << t.parse_time_ms
                  << std::setw(10) << t.validate_time_ms
                  << std::setw(10) << t.presolve_time_ms
                  << std::setw(10) << t.solve_time_ms
                  << std::setw(10) << t.total_time_ms
                  << std::setw(16) << t.final_status
                  << "\n";
    }
    std::cout << "=========================================================================================================\n\n";
}

} // namespace bharatopt
