#include <bharatopt/mps_parser.hpp>
#include <bharatopt/model_validator.hpp>
#include <bharatopt/presolve.hpp>
#include <bharatopt/revised_simplex.hpp>
#include <bharatopt/dual_revised_simplex.hpp>
#include <bharatopt/branch_and_bound.hpp>
#include <bharatopt/solution_verifier.hpp>
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace bharatopt;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: conformance_runner <file.mps> [mode: LP|MILP|AUTO]\n";
        return 1;
    }

    std::string filepath = argv[1];
    std::string mode = (argc >= 3) ? argv[2] : "AUTO";

    MpsParser parser;
    MpsParseResult parse_res = parser.parse_file(filepath);

    if (parse_res.status != MpsParseStatus::SUCCESS) {
        std::cout << "{\n"
                  << "  \"status\": \"PARSE_ERROR\",\n"
                  << "  \"error_message\": \"" << parse_res.error_message << "\",\n"
                  << "  \"objective\": 0.0,\n"
                  << "  \"solve_time_ms\": 0.0\n"
                  << "}\n";
        return 0;
    }

    LPModel model = parse_res.model;
    bool is_milp = false;
    for (const auto& v : model.variables()) {
        if (v.type == VariableType::INTEGER || v.type == VariableType::BINARY) {
            is_milp = true;
            break;
        }
    }

    bool force_lp_relaxation = (mode == "LP");

    if (force_lp_relaxation && is_milp) {
        // Relax integrality to continuous
        LPModel relaxed_model = model;
        for (size_t j = 0; j < relaxed_model.num_variables(); ++j) {
            auto& v = relaxed_model.get_variable(static_cast<index_t>(j));
            if (v.type == VariableType::INTEGER || v.type == VariableType::BINARY) {
                v.type = VariableType::CONTINUOUS;
            }
        }
        model = relaxed_model;
        is_milp = false;
    }

    PresolveEngine presolver;
    PresolveResult presolve_res = presolver.presolve(model);
    LPModel active_model = (presolve_res.status == PresolveStatus::SUCCESS) ? presolve_res.reduced_model : model;

    std::string solve_status = "UNKNOWN";
    real_t objective = 0.0;
    std::vector<real_t> solution;

    auto t_start = std::chrono::high_resolution_clock::now();

    if (is_milp && mode != "LP") {
        BranchAndBoundEngine bnb_solver;
        BnBResult b_res = bnb_solver.solve(active_model);
        if (b_res.status == BnBSolverStatus::OPTIMAL) solve_status = "OPTIMAL";
        else if (b_res.status == BnBSolverStatus::INFEASIBLE) solve_status = "INFEASIBLE";
        else if (b_res.status == BnBSolverStatus::LIMIT_REACHED) solve_status = "LIMIT_REACHED";
        else solve_status = "NUMERICAL_FAILURE";

        objective = b_res.objective_value;
        solution = b_res.solution;
    } else {
        DualRevisedSimplex dual_solver;
        DualRevisedSimplexResult d_res = dual_solver.solve(active_model);
        if (d_res.status == DualRevisedSimplexStatus::OPTIMAL) {
            solve_status = "OPTIMAL";
            objective = d_res.objective_value;
            solution = d_res.primal_solution;
        } else if (d_res.status == DualRevisedSimplexStatus::INFEASIBLE) {
            solve_status = "INFEASIBLE";
        } else if (d_res.status == DualRevisedSimplexStatus::UNBOUNDED) {
            solve_status = "UNBOUNDED";
        } else {
            RevisedSimplex rev_solver;
            RevisedSimplexResult r_res = rev_solver.solve(active_model);
            if (r_res.status == RevisedSimplexStatus::OPTIMAL) solve_status = "OPTIMAL";
            else if (r_res.status == RevisedSimplexStatus::INFEASIBLE) solve_status = "INFEASIBLE";
            else if (r_res.status == RevisedSimplexStatus::UNBOUNDED) solve_status = "UNBOUNDED";
            else solve_status = "NUMERICAL_FAILURE";

            objective = r_res.objective_value;
            solution = r_res.primal_solution;
        }
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    double solve_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    if (solve_status == "OPTIMAL" && presolve_res.status == PresolveStatus::SUCCESS) {
        solution = presolve_res.postsolve.recover_solution(solution);
        objective = presolve_res.postsolve.compute_original_objective(solution);
    }

    SolutionVerifier verifier;
    auto v_res = verifier.verify(parse_res.model, solution, objective);
    bool verified = (solve_status == "OPTIMAL") ? v_res.verified : false;
    if (!v_res.verified && solve_status == "OPTIMAL") {
        std::cerr << "Verifier diagnostic for " << filepath << ": " << v_res.status_message << "\n";
        for (const auto& msg : v_res.diagnostic_messages) {
            std::cerr << "  diag: " << msg << "\n";
        }
    }

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "{\n"
              << "  \"status\": \"" << solve_status << "\",\n"
              << "  \"objective\": " << objective << ",\n"
              << "  \"solve_time_ms\": " << solve_time_ms << ",\n"
              << "  \"verified\": " << (verified ? "true" : "false") << "\n"
              << "}\n";

    return 0;
}
