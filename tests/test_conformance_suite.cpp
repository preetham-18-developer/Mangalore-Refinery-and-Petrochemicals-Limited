#include <bharatopt/mps_parser.hpp>
#include <bharatopt/model_validator.hpp>
#include <bharatopt/presolve.hpp>
#include <bharatopt/execution_router.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/dual_revised_simplex.hpp>
#include <bharatopt/branch_and_bound.hpp>
#include <bharatopt/solution_verifier.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>

using namespace bharatopt;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mps_filepath> [mode: LP|MILP]\n";
        return 1;
    }

    std::string filepath = argv[1];
    std::string mode = (argc >= 3) ? argv[2] : "AUTO";

    auto t0 = std::chrono::high_resolution_clock::now();
    MpsParser parser;
    MpsParseResult parse_res = parser.parse_file(filepath);

    if (parse_res.status != MpsParseStatus::SUCCESS) {
        std::cout << "{\n"
                  << "  \"status\": \"PARSE_ERROR\",\n"
                  << "  \"error\": \"" << parse_res.error_message << "\",\n"
                  << "  \"objective\": 0.0,\n"
                  << "  \"cols\": 0,\n"
                  << "  \"rows\": 0,\n"
                  << "  \"nnz\": 0,\n"
                  << "  \"solve_time_ms\": 0.0\n"
                  << "}\n";
        return 0;
    }

    ModelValidator validator;
    ValidationResult val_res = validator.validate(parse_res.model);

    PresolveEngine presolver;
    PresolveResult p_res = presolver.presolve(parse_res.model);

    LPModel active_model = (p_res.status == PresolveStatus::SUCCESS) ? p_res.reduced_model : parse_res.model;

    bool is_milp = false;
    if (mode == "MILP") {
        is_milp = true;
    } else if (mode == "LP") {
        is_milp = false;
    } else {
        for (const auto& v : active_model.variables()) {
            if (v.type == VariableType::INTEGER || v.type == VariableType::BINARY) {
                is_milp = true;
                break;
            }
        }
    }

    std::string status_str = "NUMERICAL_FAILURE";
    real_t objective = 0.0;
    std::vector<real_t> solution;

    if (is_milp) {
        BnBConfig bnb_cfg;
        bnb_cfg.time_limit_ms = 30000.0;
        BranchAndBoundEngine bnb(bnb_cfg);
        BnBResult b_res = bnb.solve(active_model);

        if (b_res.status == BnBSolverStatus::OPTIMAL) status_str = "OPTIMAL";
        else if (b_res.status == BnBSolverStatus::INFEASIBLE) status_str = "INFEASIBLE";
        else if (b_res.status == BnBSolverStatus::UNRESOLVED && b_res.status_message.find("unbounded") != std::string::npos) status_str = "UNBOUNDED";
        else if (b_res.status == BnBSolverStatus::LIMIT_REACHED) status_str = "TIME_LIMIT_REACHED";
        else status_str = "OTHER";

        objective = b_res.objective_value;
        solution = b_res.solution;
    } else {
        DualRevisedSimplexOptions d_opts;
        d_opts.max_iterations = 20000;
        DualRevisedSimplex d_solver(d_opts);
        DualRevisedSimplexResult d_res = d_solver.solve(active_model);

        if (d_res.status == DualRevisedSimplexStatus::OPTIMAL) {
            status_str = "OPTIMAL";
            objective = d_res.objective_value;
            solution = d_res.primal_solution;
        } else if (d_res.status == DualRevisedSimplexStatus::INFEASIBLE) {
            status_str = "INFEASIBLE";
        } else if (d_res.status == DualRevisedSimplexStatus::UNBOUNDED) {
            status_str = "UNBOUNDED";
        } else {
            RevisedSimplexOptions r_opts;
            r_opts.max_iterations = 20000;
            r_opts.entering_rule = EnteringRule::MOST_NEGATIVE;
            RevisedSimplex r_solver(r_opts);
            RevisedSimplexResult r_res = r_solver.solve(active_model);
            if (r_res.status == RevisedSimplexStatus::OPTIMAL) {
                status_str = "OPTIMAL";
                objective = r_res.objective_value;
                solution = r_res.primal_solution;
            } else if (r_res.status == RevisedSimplexStatus::INFEASIBLE) {
                status_str = "INFEASIBLE";
            } else if (r_res.status == RevisedSimplexStatus::UNBOUNDED) {
                status_str = "UNBOUNDED";
            } else {
                RevisedSimplexOptions r_opts_b;
                r_opts_b.max_iterations = 20000;
                r_opts_b.entering_rule = EnteringRule::BLANDS_RULE;
                RevisedSimplex r_solver_b(r_opts_b);
                RevisedSimplexResult r_res_b = r_solver_b.solve(active_model);
                if (r_res_b.status == RevisedSimplexStatus::OPTIMAL) {
                    status_str = "OPTIMAL";
                    objective = r_res_b.objective_value;
                    solution = r_res_b.primal_solution;
                } else if (r_res_b.status == RevisedSimplexStatus::INFEASIBLE) {
                    status_str = "INFEASIBLE";
                } else if (r_res_b.status == RevisedSimplexStatus::UNBOUNDED) {
                    status_str = "UNBOUNDED";
                } else {
                    RoutingConfiguration r_cfg;
                    r_cfg.presolve_enabled = true;
                    r_cfg.fallback_enabled = true;
                    ExecutionRouter router(r_cfg);
                    RoutedSolveResult ex_res = router.solve(parse_res.model);
                    status_str = ex_res.status;
                    objective = ex_res.objective_value;
                    solution = ex_res.primal_solution;
                }
            }
        }
    }

    if (status_str == "OPTIMAL" && p_res.status == PresolveStatus::SUCCESS && !solution.empty()) {
        std::vector<real_t> full_sol = p_res.postsolve.recover_solution(solution);
        objective = p_res.postsolve.compute_original_objective(full_sol);
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    size_t nnz = 0;
    for (const auto& c : parse_res.model.constraints()) nnz += c.terms.size();

    std::cout << std::setprecision(12) << "{\n"
              << "  \"status\": \"" << status_str << "\",\n"
              << "  \"objective\": " << objective << ",\n"
              << "  \"cols\": " << parse_res.cols_parsed << ",\n"
              << "  \"rows\": " << parse_res.rows_parsed << ",\n"
              << "  \"nnz\": " << nnz << ",\n"
              << "  \"solve_time_ms\": " << total_ms << "\n"
              << "}\n";

    return 0;
}
