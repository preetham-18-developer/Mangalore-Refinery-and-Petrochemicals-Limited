#ifndef BHARATOPT_BASIS_UPDATE_HPP
#define BHARATOPT_BASIS_UPDATE_HPP

#include <vector>
#include <memory>
#include <cstddef>
#include <bharatopt/config.hpp>
#include <bharatopt/revised_simplex.hpp>

namespace bharatopt {

/**
 * Elementary Basis Update Representation (Eta Vector).
 * Represents E_k^(-1) = I + (\eta_k - e_p) e_p^T
 * where p is the pivot row position, and \eta_k is the transformed column.
 */
struct EtaVector {
    size_t pivot_pos{0};                // Row index p in basis
    index_t entering_var{-1};           // Entering variable index
    index_t leaving_var{-1};            // Leaving variable index
    real_t pivot_val{1.0};              // Value of pivot element d_B[p]
    std::vector<index_t> sparse_indices;// Non-zero row indices of eta vector
    std::vector<real_t> sparse_values;  // Non-zero values of eta vector
};

struct BasisUpdateStats {
    size_t total_pivots{0};
    size_t full_refactorisations{0};
    size_t incremental_updates{0};
    size_t current_update_chain_length{0};
    size_t max_update_chain_length{0};
    real_t last_solve_residual{0.0};
};

struct BasisUpdateOptions {
    size_t max_eta_updates{50};
    real_t pivot_tolerance{DEFAULT_PIVOT_TOLERANCE};
    real_t residual_tolerance{1e-4};
    bool enable_incremental{true};
};

/**
 * BasisUpdateManager.
 * Manages incremental Product-Form of Inverse (Eta-vector) basis updates
 * on top of a base IBasisSolver (DenseBasisSolver or SparseLUBasisSolver).
 */
class BasisUpdateManager : public IBasisSolver {
public:
    explicit BasisUpdateManager(std::unique_ptr<IBasisSolver> base_solver,
                               BasisUpdateOptions options = {});

    // Full refactorisation of current basis
    bool factorize(const StandardFormLP& lp, const Basis& basis) override;

    // Primal solve B_k x = rhs using base solver + Eta chain
    bool solve_primal(const std::vector<real_t>& rhs, std::vector<real_t>& x) const override;

    // Transpose/Dual solve B_k^T y = rhs using Eta transpose chain + base solver
    bool solve_dual(const std::vector<real_t>& rhs, std::vector<real_t>& y) const override;

    // Add an incremental Eta update after a pivot
    bool add_update(size_t pivot_pos,
                    index_t entering_var,
                    index_t leaving_var,
                    const std::vector<real_t>& d_B);

    // Force full refactorisation and clear Eta chain
    bool force_refactorize(const StandardFormLP& lp, const Basis& basis);

    // Query stats & options
    const BasisUpdateStats& stats() const { return stats_; }
    const BasisUpdateOptions& options() const { return options_; }
    size_t eta_count() const { return etas_.size(); }
    void clear_updates();

private:
    std::unique_ptr<IBasisSolver> base_solver_;
    BasisUpdateOptions options_;
    BasisUpdateStats stats_;

    size_t m_{0};
    std::vector<EtaVector> etas_;
};

} // namespace bharatopt

#endif // BHARATOPT_BASIS_UPDATE_HPP
