#include <bharatopt/educational_simplex.hpp>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <sstream>

namespace bharatopt {

void Tableau::print(std::ostream& os) const {
    os << "========================================================\n";
    os << " EDUCATIONAL SIMPLEX TABLEAU (" << num_rows << " rows x " << num_cols << " cols)\n";
    os << "========================================================\n";

    // Header row
    os << std::setw(10) << "Basis" << " | ";
    for (size_t j = 0; j < num_cols; ++j) {
        std::string name = (j < col_names.size()) ? col_names[j] : ("x" + std::to_string(j));
        os << std::setw(12) << name;
    }
    os << " | " << std::setw(12) << "RHS" << "\n";
    os << std::string(14 + (num_cols + 1) * 12, '-') << "\n";

    // Constraint rows
    for (size_t i = 0; i < num_rows; ++i) {
        index_t bvar = (i < basis.size()) ? basis[i] : -1;
        std::string bname = (bvar >= 0 && static_cast<size_t>(bvar) < col_names.size())
                            ? col_names[bvar] : ("r" + std::to_string(i));
        os << std::setw(10) << bname << " | ";
        for (size_t j = 0; j < num_cols; ++j) {
            os << std::setw(12) << std::fixed << std::setprecision(4) << matrix[i][j];
        }
        os << " | " << std::setw(12) << std::fixed << std::setprecision(4) << matrix[i][num_cols] << "\n";
    }

    os << std::string(14 + (num_cols + 1) * 12, '-') << "\n";

    // Objective row
    os << std::setw(10) << "Obj (z)" << " | ";
    for (size_t j = 0; j < num_cols; ++j) {
        os << std::setw(12) << std::fixed << std::setprecision(4) << matrix[num_rows][j];
    }
    os << " | " << std::setw(12) << std::fixed << std::setprecision(4) << matrix[num_rows][num_cols] << "\n";
    os << "========================================================\n";
}

EducationalSimplex::EducationalSimplex(SimplexOptions options)
    : options_(options) {}

Tableau EducationalSimplex::create_initial_tableau(const LPModel& model) {
    Tableau tableau;
    tableau.original_sense = model.sense();
    tableau.original_obj_offset = model.obj_offset();

    size_t orig_n = model.num_variables();
    tableau.orig_var_map.clear();
    tableau.var_shifts.assign(orig_n, 0.0);

    // Standardize decision variables: handle finite lower bounds
    std::vector<std::string> var_col_names;
    std::vector<index_t> col_to_orig;
    std::vector<real_t> col_obj_coeffs;

    for (size_t j = 0; j < orig_n; ++j) {
        const auto& var = model.get_variable(static_cast<index_t>(j));
        real_t lb = var.lower_bound;

        if (var.is_free()) {
            // Split free variable x_j = x_j^+ - x_j^-
            std::string p_name = var.name + "+";
            std::string m_name = var.name + "-";

            var_col_names.push_back(p_name);
            col_to_orig.push_back(static_cast<index_t>(j));
            col_obj_coeffs.push_back(var.obj_coeff);

            var_col_names.push_back(m_name);
            col_to_orig.push_back(static_cast<index_t>(j));
            col_obj_coeffs.push_back(-var.obj_coeff);
        } else {
            real_t shift = 0.0;
            if (!std::isinf(lb) && lb != 0.0) {
                shift = lb;
            }
            tableau.var_shifts[j] = shift;

            var_col_names.push_back(var.name);
            col_to_orig.push_back(static_cast<index_t>(j));
            col_obj_coeffs.push_back(var.obj_coeff);
        }
    }

    // Process constraints to determine total rows and auxiliary variables
    struct StandardRow {
        std::vector<std::pair<size_t, real_t>> coeffs; // (standard_var_col, coeff)
        real_t rhs{0.0};
        ConstraintSense sense{ConstraintSense::LESS_EQUAL};
        std::string name;
    };

    std::vector<StandardRow> std_rows;

    for (size_t i = 0; i < model.num_constraints(); ++i) {
        const auto& cons = model.get_constraint(static_cast<index_t>(i));

        auto process_single_row = [&](ConstraintSense sense, real_t raw_rhs, const std::string& row_name) {
            StandardRow srow;
            srow.sense = sense;
            srow.rhs = raw_rhs;
            srow.name = row_name;

            for (const auto& term : cons.terms) {
                index_t orig_j = term.first;
                real_t val = term.second;
                const auto& var = model.get_variable(orig_j);

                if (var.is_free()) {
                    // Find standard cols for x+ and x-
                    size_t p_col = 0, m_col = 0;
                    for (size_t c = 0; c < col_to_orig.size(); ++c) {
                        if (col_to_orig[c] == orig_j) {
                            if (var_col_names[c].back() == '+') p_col = c;
                            else if (var_col_names[c].back() == '-') m_col = c;
                        }
                    }
                    srow.coeffs.push_back({p_col, val});
                    srow.coeffs.push_back({m_col, -val});
                } else {
                    size_t std_c = 0;
                    for (size_t c = 0; c < col_to_orig.size(); ++c) {
                        if (col_to_orig[c] == orig_j) {
                            std_c = c;
                            break;
                        }
                    }
                    srow.coeffs.push_back({std_c, val});
                    // Shift RHS by a_ij * l_j
                    if (tableau.var_shifts[orig_j] != 0.0) {
                        srow.rhs -= val * tableau.var_shifts[orig_j];
                    }
                }
            }
            std_rows.push_back(srow);
        };

        if (cons.sense == ConstraintSense::RANGED) {
            process_single_row(ConstraintSense::GREATER_EQUAL, cons.rhs, cons.name + "_lower");
            process_single_row(ConstraintSense::LESS_EQUAL, cons.range_upper, cons.name + "_upper");
        } else {
            process_single_row(cons.sense, cons.rhs, cons.name);
        }
    }

    // Add explicit upper bound constraints for variables with finite u_j < infinity
    for (size_t j = 0; j < orig_n; ++j) {
        const auto& var = model.get_variable(static_cast<index_t>(j));
        if (!var.is_free() && var.upper_bound < BHARATOPT_INFINITY && var.upper_bound != var.lower_bound) {
            StandardRow ub_row;
            ub_row.sense = ConstraintSense::LESS_EQUAL;
            ub_row.rhs = var.upper_bound - tableau.var_shifts[j];
            ub_row.name = var.name + "_ub";

            size_t std_c = 0;
            for (size_t c = 0; c < col_to_orig.size(); ++c) {
                if (col_to_orig[c] == static_cast<index_t>(j)) {
                    std_c = c;
                    break;
                }
            }
            ub_row.coeffs.push_back({std_c, 1.0});
            std_rows.push_back(ub_row);
        }
    }

    // Determine auxiliary variables (slacks, surplus, artificials)
    tableau.num_rows = std_rows.size();
    size_t decision_cols = var_col_names.size();

    tableau.col_names = var_col_names;
    tableau.orig_var_map = col_to_orig;
    tableau.is_slack.assign(decision_cols, false);
    tableau.is_surplus.assign(decision_cols, false);
    tableau.is_artificial.assign(decision_cols, false);

    struct AuxiliaryVar {
        std::string name;
        size_t row_idx;
        bool is_slack{false};
        bool is_surplus{false};
        bool is_artificial{false};
    };

    std::vector<AuxiliaryVar> aux_vars;
    std::vector<index_t> initial_basis(tableau.num_rows, -1);

    for (size_t i = 0; i < std_rows.size(); ++i) {
        auto& srow = std_rows[i];

        // Ensure RHS is non-negative: if rhs < 0, flip sign and sense
        if (srow.rhs < 0.0) {
            srow.rhs = -srow.rhs;
            for (auto& term : srow.coeffs) {
                term.second = -term.second;
            }
            if (srow.sense == ConstraintSense::LESS_EQUAL) {
                srow.sense = ConstraintSense::GREATER_EQUAL;
            } else if (srow.sense == ConstraintSense::GREATER_EQUAL) {
                srow.sense = ConstraintSense::LESS_EQUAL;
            }
        }

        if (srow.sense == ConstraintSense::LESS_EQUAL) {
            std::string sname = "s_" + srow.name;
            aux_vars.push_back({sname, i, true, false, false});
        } else if (srow.sense == ConstraintSense::GREATER_EQUAL) {
            std::string sname = "sur_" + srow.name;
            std::string aname = "art_" + srow.name;
            aux_vars.push_back({sname, i, false, true, false});
            aux_vars.push_back({aname, i, false, false, true});
        } else if (srow.sense == ConstraintSense::EQUAL) {
            std::string aname = "art_" + srow.name;
            aux_vars.push_back({aname, i, false, false, true});
        }
    }

    // Append auxiliary columns to tableau structure
    for (const auto& aux : aux_vars) {
        size_t col_idx = tableau.col_names.size();
        tableau.col_names.push_back(aux.name);
        tableau.orig_var_map.push_back(-1);
        tableau.is_slack.push_back(aux.is_slack);
        tableau.is_surplus.push_back(aux.is_surplus);
        tableau.is_artificial.push_back(aux.is_artificial);

        if (aux.is_slack || aux.is_artificial) {
            initial_basis[aux.row_idx] = static_cast<index_t>(col_idx);
        }
    }

    tableau.num_cols = tableau.col_names.size();
    tableau.basis = initial_basis;

    // Allocate matrix (m+1) x (n+1)
    tableau.matrix.assign(tableau.num_rows + 1, std::vector<real_t>(tableau.num_cols + 1, 0.0));

    // Fill constraint rows
    for (size_t i = 0; i < std_rows.size(); ++i) {
        const auto& srow = std_rows[i];
        for (const auto& term : srow.coeffs) {
            tableau.matrix[i][term.first] = term.second;
        }
        tableau.matrix[i][tableau.num_cols] = srow.rhs;
    }

    // Fill auxiliary column coefficients in matrix
    size_t aux_start = decision_cols;
    for (const auto& aux : aux_vars) {
        if (aux.is_slack) {
            tableau.matrix[aux.row_idx][aux_start] = 1.0;
        } else if (aux.is_surplus) {
            tableau.matrix[aux.row_idx][aux_start] = -1.0;
        } else if (aux.is_artificial) {
            tableau.matrix[aux.row_idx][aux_start] = 1.0;
        }
        aux_start++;
    }

    // Fill Objective Row (row m) for Phase II Objective
    // Maximization: z - sum(c_j * x_j) = obj_offset
    // Row m stores: matrix[m][j] = -c_j for max, or +c_j for min (converted to max by -c)
    real_t sense_factor = (model.sense() == ObjectiveSense::MAXIMIZE) ? 1.0 : -1.0;
    real_t total_obj_offset = model.obj_offset();

    for (size_t c = 0; c < decision_cols; ++c) {
        real_t c_j = col_obj_coeffs[c] * sense_factor;
        tableau.matrix[tableau.num_rows][c] = -c_j;
    }

    // Adjust objective offset for variable shifts
    for (size_t j = 0; j < orig_n; ++j) {
        if (tableau.var_shifts[j] != 0.0) {
            total_obj_offset += model.get_variable(static_cast<index_t>(j)).obj_coeff * tableau.var_shifts[j];
        }
    }

    // Objective value in tableau is -z, so matrix[m][n] = - (total_obj_offset * sense_factor)
    tableau.matrix[tableau.num_rows][tableau.num_cols] = -(total_obj_offset * sense_factor);

    return tableau;
}

void EducationalSimplex::canonicalize_objective(Tableau& tableau) {
    // Zero out basic variable entries in the objective row
    for (size_t i = 0; i < tableau.num_rows; ++i) {
        index_t bvar = tableau.basis[i];
        if (bvar >= 0 && static_cast<size_t>(bvar) < tableau.num_cols) {
            real_t c_b = tableau.matrix[tableau.num_rows][bvar];
            if (std::abs(c_b) > options_.zero_tolerance) {
                for (size_t j = 0; j <= tableau.num_cols; ++j) {
                    tableau.matrix[tableau.num_rows][j] -= c_b * tableau.matrix[i][j];
                }
            }
        }
    }
}

bool EducationalSimplex::pivot(Tableau& tableau, size_t pivot_row, size_t pivot_col) {
    if (pivot_row >= tableau.num_rows || pivot_col >= tableau.num_cols) {
        return false;
    }

    real_t pivot_elem = tableau.matrix[pivot_row][pivot_col];
    if (std::abs(pivot_elem) < options_.pivot_tolerance) {
        return false; // Near-zero pivot element
    }

    // Step 1: Normalize pivot row
    for (size_t j = 0; j <= tableau.num_cols; ++j) {
        tableau.matrix[pivot_row][j] /= pivot_elem;
    }

    // Step 2: Eliminate pivot column entries from all other rows (including objective row m)
    for (size_t i = 0; i <= tableau.num_rows; ++i) {
        if (i != pivot_row) {
            real_t factor = tableau.matrix[i][pivot_col];
            if (std::abs(factor) > options_.zero_tolerance) {
                for (size_t j = 0; j <= tableau.num_cols; ++j) {
                    tableau.matrix[i][j] -= factor * tableau.matrix[pivot_row][j];
                }
            }
        }
    }

    // Step 3: Update basis vector
    tableau.basis[pivot_row] = static_cast<index_t>(pivot_col);
    return true;
}

index_t EducationalSimplex::select_entering_variable(const Tableau& tableau) const {
    index_t entering_col = -1;
    real_t min_reduced_cost = -options_.optimality_tolerance;

    if (options_.entering_rule == EnteringRule::BLANDS_RULE) {
        for (size_t j = 0; j < tableau.num_cols; ++j) {
            // Skip artificial columns if they are disabled/removed
            if (tableau.matrix[tableau.num_rows][j] < -options_.optimality_tolerance) {
                return static_cast<index_t>(j);
            }
        }
    } else { // MOST_NEGATIVE (Dantzig)
        for (size_t j = 0; j < tableau.num_cols; ++j) {
            real_t rc = tableau.matrix[tableau.num_rows][j];
            if (rc < min_reduced_cost) {
                min_reduced_cost = rc;
                entering_col = static_cast<index_t>(j);
            }
        }
    }

    return entering_col;
}

index_t EducationalSimplex::select_leaving_variable(const Tableau& tableau, size_t entering_col) const {
    index_t leaving_row = -1;
    real_t min_ratio = BHARATOPT_INFINITY;

    for (size_t i = 0; i < tableau.num_rows; ++i) {
        real_t a_ie = tableau.matrix[i][entering_col];
        if (a_ie > options_.pivot_tolerance) {
            real_t b_i = tableau.matrix[i][tableau.num_cols];
            // Handle numerical zero RHS
            if (std::abs(b_i) < options_.zero_tolerance) {
                b_i = 0.0;
            }

            real_t ratio = b_i / a_ie;
            if (ratio < min_ratio - options_.zero_tolerance) {
                min_ratio = ratio;
                leaving_row = static_cast<index_t>(i);
            } else if (std::abs(ratio - min_ratio) <= options_.zero_tolerance && leaving_row >= 0) {
                // Tie-breaking: Bland's rule uses smallest basic variable index
                index_t current_basic = tableau.basis[leaving_row];
                index_t candidate_basic = tableau.basis[i];
                if (candidate_basic < current_basic) {
                    leaving_row = static_cast<index_t>(i);
                }
            }
        }
    }

    return leaving_row;
}

SimplexResult EducationalSimplex::solve(const LPModel& model) {
    SimplexResult result;
    Tableau tableau = create_initial_tableau(model);

    // Check if artificial variables exist (Phase I required)
    bool has_artificials = false;
    for (bool art : tableau.is_artificial) {
        if (art) {
            has_artificials = true;
            break;
        }
    }

    // Save Phase II objective row
    std::vector<real_t> phase2_obj_row = tableau.matrix[tableau.num_rows];

    if (has_artificials) {
        // --- PHASE I SIMPLEX ---
        // Minimize sum of artificial variables -> Maximize w = - sum(a_i)
        // Row m in Phase I: 1.0 for artificial columns, 0.0 for others
        std::fill(tableau.matrix[tableau.num_rows].begin(), tableau.matrix[tableau.num_rows].end(), 0.0);
        for (size_t j = 0; j < tableau.num_cols; ++j) {
            if (tableau.is_artificial[j]) {
                tableau.matrix[tableau.num_rows][j] = 1.0;
            }
        }

        canonicalize_objective(tableau);

        size_t phase1_iters = 0;
        while (phase1_iters < options_.max_iterations) {
            index_t enter_col = select_entering_variable(tableau);
            if (enter_col < 0) {
                break; // Phase I Optimal
            }

            index_t leave_row = select_leaving_variable(tableau, static_cast<size_t>(enter_col));
            if (leave_row < 0) {
                result.status = SimplexStatus::UNBOUNDED;
                result.message = "Unbounded ray detected during Phase I";
                result.final_tableau = tableau;
                return result;
            }

            real_t pivot_val = tableau.matrix[leave_row][enter_col];
            if (!pivot(tableau, static_cast<size_t>(leave_row), static_cast<size_t>(enter_col))) {
                result.status = SimplexStatus::NUMERICAL_FAILURE;
                result.message = "Numerical failure during Phase I pivot";
                result.final_tableau = tableau;
                return result;
            }

            phase1_iters++;
            result.iterations++;

            if (options_.enable_trace) {
                SimplexIterationTrace trace_entry;
                trace_entry.iteration = result.iterations;
                trace_entry.entering_var = enter_col;
                trace_entry.leaving_var = tableau.basis[leave_row];
                trace_entry.pivot_row = leave_row;
                trace_entry.pivot_col = enter_col;
                trace_entry.pivot_element = pivot_val;
                trace_entry.objective_value = -tableau.matrix[tableau.num_rows][tableau.num_cols];
                trace_entry.details = "Phase I Pivot";
                result.trace.push_back(trace_entry);
            }
        }

        // Check Phase I feasibility: Phase I objective w must be ~0 (artificial sum = -matrix[m][n] <= tol)
        real_t artificial_sum = -tableau.matrix[tableau.num_rows][tableau.num_cols];
        if (artificial_sum > options_.optimality_tolerance) {
            result.status = SimplexStatus::INFEASIBLE;
            result.message = "Infeasible LP: Phase I artificial variable sum > 0";
            result.final_tableau = tableau;
            return result;
        }

        // Restore Phase II objective row and canonicalize
        tableau.matrix[tableau.num_rows] = phase2_obj_row;
        // Block artificial columns from re-entering by making their reduced cost infinity
        for (size_t j = 0; j < tableau.num_cols; ++j) {
            if (tableau.is_artificial[j]) {
                tableau.matrix[tableau.num_rows][j] = BHARATOPT_INFINITY;
            }
        }
        canonicalize_objective(tableau);
    } else {
        // No artificials: simply canonicalize initial Phase II objective row
        canonicalize_objective(tableau);
    }

    // --- PHASE II SIMPLEX ---
    size_t phase2_iters = 0;
    (void)phase2_iters;
    while (result.iterations < options_.max_iterations) {
        index_t enter_col = select_entering_variable(tableau);
        if (enter_col < 0) {
            result.status = SimplexStatus::OPTIMAL;
            result.message = "Optimal solution found";
            break;
        }

        index_t leave_row = select_leaving_variable(tableau, static_cast<size_t>(enter_col));
        if (leave_row < 0) {
            result.status = SimplexStatus::UNBOUNDED;
            result.message = "Unbounded LP: entering variable has no positive pivot coefficients";
            result.final_tableau = tableau;
            return result;
        }

        real_t pivot_val = tableau.matrix[leave_row][enter_col];
        if (!pivot(tableau, static_cast<size_t>(leave_row), static_cast<size_t>(enter_col))) {
            result.status = SimplexStatus::NUMERICAL_FAILURE;
            result.message = "Numerical failure during Phase II pivot";
            result.final_tableau = tableau;
            return result;
        }

        result.iterations++;
        phase2_iters++;

        if (options_.enable_trace) {
            SimplexIterationTrace trace_entry;
            trace_entry.iteration = result.iterations;
            trace_entry.entering_var = enter_col;
            trace_entry.leaving_var = tableau.basis[leave_row];
            trace_entry.pivot_row = leave_row;
            trace_entry.pivot_col = enter_col;
            trace_entry.pivot_element = pivot_val;
            trace_entry.objective_value = -tableau.matrix[tableau.num_rows][tableau.num_cols];
            trace_entry.details = "Phase II Pivot";
            result.trace.push_back(trace_entry);
        }
    }

    if (result.iterations >= options_.max_iterations && result.status != SimplexStatus::OPTIMAL) {
        result.status = SimplexStatus::ITERATION_LIMIT;
        result.message = "Iteration limit reached before optimality";
    }

    result.final_tableau = tableau;

    // --- SOLUTION EXTRACTION ---
    if (result.status == SimplexStatus::OPTIMAL) {
        size_t orig_n = model.num_variables();
        result.primal_solution.assign(orig_n, 0.0);

        // Read basic variable values from tableau RHS
        for (size_t i = 0; i < tableau.num_rows; ++i) {
            index_t bvar = tableau.basis[i];
            if (bvar >= 0 && static_cast<size_t>(bvar) < tableau.num_cols) {
                index_t orig_j = tableau.orig_var_map[bvar];
                if (orig_j >= 0 && static_cast<size_t>(orig_j) < orig_n) {
                    real_t val = tableau.matrix[i][tableau.num_cols];
                    const auto& var = model.get_variable(orig_j);

                    if (var.is_free()) {
                        if (tableau.col_names[bvar].back() == '+') {
                            result.primal_solution[orig_j] += val;
                        } else if (tableau.col_names[bvar].back() == '-') {
                            result.primal_solution[orig_j] -= val;
                        }
                    } else {
                        result.primal_solution[orig_j] = val + tableau.var_shifts[orig_j];
                    }
                }
            }
        }

        // For non-basic original decision variables: if shifted, set to shift value l_j
        for (size_t j = 0; j < orig_n; ++j) {
            const auto& var = model.get_variable(static_cast<index_t>(j));
            if (!var.is_free() && tableau.var_shifts[j] != 0.0) {
                // If not assigned from basis (was zero in standard form), it equals shift
                bool is_in_basis = false;
                for (size_t i = 0; i < tableau.num_rows; ++i) {
                    index_t bvar = tableau.basis[i];
                    if (bvar >= 0 && tableau.orig_var_map[bvar] == static_cast<index_t>(j)) {
                        is_in_basis = true;
                        break;
                    }
                }
                if (!is_in_basis) {
                    result.primal_solution[j] = tableau.var_shifts[j];
                }
            }
        }

        // Recompute original objective value
        result.objective_value = recompute_original_objective(model, result.primal_solution);
    }

    return result;
}

bool EducationalSimplex::verify_solution_feasibility(const LPModel& model,
                                                   const std::vector<real_t>& solution,
                                                   real_t tol) const {
    if (solution.size() != model.num_variables()) {
        return false;
    }

    // Check variable bounds
    for (size_t j = 0; j < model.num_variables(); ++j) {
        const auto& var = model.get_variable(static_cast<index_t>(j));
        real_t val = solution[j];
        if (val < var.lower_bound - tol || val > var.upper_bound + tol) {
            return false;
        }
    }

    // Check constraints
    for (size_t i = 0; i < model.num_constraints(); ++i) {
        const auto& cons = model.get_constraint(static_cast<index_t>(i));
        real_t lhs = 0.0;
        for (const auto& term : cons.terms) {
            lhs += term.second * solution[term.first];
        }

        if (cons.sense == ConstraintSense::LESS_EQUAL && lhs > cons.rhs + tol) {
            return false;
        }
        if (cons.sense == ConstraintSense::GREATER_EQUAL && lhs < cons.rhs - tol) {
            return false;
        }
        if (cons.sense == ConstraintSense::EQUAL && std::abs(lhs - cons.rhs) > tol) {
            return false;
        }
        if (cons.sense == ConstraintSense::RANGED && (lhs < cons.rhs - tol || lhs > cons.range_upper + tol)) {
            return false;
        }
    }

    return true;
}

real_t EducationalSimplex::recompute_original_objective(const LPModel& model,
                                                       const std::vector<real_t>& solution) const {
    real_t obj = model.obj_offset();
    for (size_t j = 0; j < model.num_variables(); ++j) {
        obj += model.get_variable(static_cast<index_t>(j)).obj_coeff * solution[j];
    }
    return obj;
}

} // namespace bharatopt
