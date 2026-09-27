#include <bharatopt/basis_update.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace bharatopt {

BasisUpdateManager::BasisUpdateManager(std::unique_ptr<IBasisSolver> base_solver,
                                       BasisUpdateOptions options)
    : base_solver_(std::move(base_solver)), options_(options) {
    if (!base_solver_) {
        base_solver_ = std::make_unique<DenseBasisSolver>();
    }
}

void BasisUpdateManager::clear_updates() {
    etas_.clear();
    stats_.current_update_chain_length = 0;
}

bool BasisUpdateManager::force_refactorize(const StandardFormLP& lp, const Basis& basis) {
    clear_updates();
    m_ = lp.num_rows;
    bool ok = base_solver_->factorize(lp, basis);
    if (ok) {
        stats_.full_refactorisations++;
    }
    return ok;
}

bool BasisUpdateManager::factorize(const StandardFormLP& lp, const Basis& basis) {
    return force_refactorize(lp, basis);
}

bool BasisUpdateManager::add_update(size_t pivot_pos,
                                    index_t entering_var,
                                    index_t leaving_var,
                                    const std::vector<real_t>& d_B) {
    m_ = d_B.size();
    stats_.total_pivots++;

    if (pivot_pos >= m_) {
        return false;
    }

    real_t gamma = d_B[pivot_pos];
    if (std::abs(gamma) < options_.pivot_tolerance) {
        return false; // Pivot element too small -> trigger refactorisation
    }

    if (options_.enable_incremental && etas_.size() >= options_.max_eta_updates) {
        return false; // Reached maximum update chain length -> trigger refactorisation
    }

    if (!options_.enable_incremental) {
        return false; // Incremental updates disabled -> always refactorise
    }

    EtaVector eta;
    eta.pivot_pos = pivot_pos;
    eta.entering_var = entering_var;
    eta.leaving_var = leaving_var;
    eta.pivot_val = gamma;

    for (size_t i = 0; i < m_; ++i) {
        real_t val = (i == pivot_pos) ? (1.0 / gamma) : (-d_B[i] / gamma);
        if (std::abs(val) > DEFAULT_ZERO_TOLERANCE) {
            eta.sparse_indices.push_back(static_cast<index_t>(i));
            eta.sparse_values.push_back(val);
        }
    }

    etas_.push_back(std::move(eta));
    stats_.incremental_updates++;
    stats_.current_update_chain_length = etas_.size();
    stats_.max_update_chain_length = std::max(stats_.max_update_chain_length, etas_.size());

    return true;
}

bool BasisUpdateManager::solve_primal(const std::vector<real_t>& rhs, std::vector<real_t>& x) const {
    if (!base_solver_->solve_primal(rhs, x)) {
        return false;
    }

    // Apply Eta updates sequentially: x_k = E_k^(-1) x_{k-1}
    for (const auto& eta : etas_) {
        size_t p = eta.pivot_pos;
        real_t v_p = x[p];

        for (size_t k = 0; k < eta.sparse_indices.size(); ++k) {
            index_t idx = eta.sparse_indices[k];
            real_t val = eta.sparse_values[k];
            if (static_cast<size_t>(idx) == p) {
                x[p] = val * v_p;
            } else {
                x[idx] += val * v_p;
            }
        }
    }

    return true;
}

bool BasisUpdateManager::solve_dual(const std::vector<real_t>& rhs, std::vector<real_t>& y) const {
    std::vector<real_t> w = rhs;

    // Apply Eta transpose updates in reverse order: w_{k-1} = (E_k^(-1))^T w_k
    for (int l = static_cast<int>(etas_.size()) - 1; l >= 0; --l) {
        const auto& eta = etas_[l];
        size_t p = eta.pivot_pos;

        real_t dot = 0.0;
        for (size_t k = 0; k < eta.sparse_indices.size(); ++k) {
            index_t idx = eta.sparse_indices[k];
            real_t val = eta.sparse_values[k];
            dot += val * w[idx];
        }
        w[p] = dot;
    }

    return base_solver_->solve_dual(w, y);
}

} // namespace bharatopt
