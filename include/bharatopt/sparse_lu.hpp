#ifndef BHARATOPT_SPARSE_LU_HPP
#define BHARATOPT_SPARSE_LU_HPP

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <bharatopt/config.hpp>
#include <bharatopt/sparse_matrix.hpp>
#include <bharatopt/revised_simplex.hpp>

namespace bharatopt {

/**
 * Statistics structure tracking Sparse LU factorization metrics.
 */
struct SparseLUStats {
    size_t matrix_dim{0};         // m x m
    size_t original_basis_nnz{0}; // Original non-zeros in B
    size_t l_nnz{0};              // Non-zeros in L factor (including diagonal)
    size_t u_nnz{0};              // Non-zeros in U factor
    size_t total_factor_nnz{0};   // Total non-zeros in L + U (excluding duplicate unit diagonal)
    int64_t fill_in_nnz{0};       // total_factor_nnz - original_basis_nnz
    real_t fill_in_ratio{1.0};    // total_factor_nnz / original_basis_nnz
    real_t pivot_min{0.0};        // Minimum absolute pivot value
    real_t pivot_max{0.0};        // Maximum absolute pivot value
};

/**
 * Sparse LU Factorization Engine.
 * Computes P B = L U where:
 * - B is m x m basis matrix extracted sparsely
 * - P is row permutation vector
 * - L is unit lower triangular sparse matrix
 * - U is non-unit upper triangular sparse matrix
 */
class SparseLUBasisSolver : public IBasisSolver {
public:
    explicit SparseLUBasisSolver(real_t pivot_tol = DEFAULT_PIVOT_TOLERANCE);

    // Factorize basis matrix extracted from StandardFormLP and Basis
    bool factorize(const StandardFormLP& lp, const Basis& basis) override;

    // Direct factorization from an arbitrary CSRMatrix (for standalone unit testing)
    bool factorize_matrix(const CSRMatrix& B_matrix);

    // Solve primal system B x = rhs
    bool solve_primal(const std::vector<real_t>& rhs, std::vector<real_t>& x) const override;

    // Solve dual (transpose) system B^T y = rhs
    bool solve_dual(const std::vector<real_t>& rhs, std::vector<real_t>& y) const override;

    // Access factorization statistics and metadata
    const SparseLUStats& stats() const { return stats_; }
    real_t pivot_tolerance() const { return pivot_tol_; }
    void set_pivot_tolerance(real_t tol) { pivot_tol_ = tol; }

    // Standalone sparse basis extraction utility
    static CSRMatrix extract_sparse_basis(const StandardFormLP& lp, const Basis& basis);

private:
    real_t pivot_tol_{DEFAULT_PIVOT_TOLERANCE};
    size_t m_{0};
    SparseLUStats stats_;

    // Permutations
    std::vector<size_t> P_row_;     // P_row[k] = original row index placed at permuted row k
    std::vector<size_t> P_inv_;     // P_inv[orig_row] = permuted row index

    // Sparse L Factor (Unit lower triangular: L_row_ptr, L_col_idx, L_val)
    std::vector<size_t> L_row_ptr_;
    std::vector<size_t> L_col_idx_;
    std::vector<real_t> L_val_;

    // Sparse U Factor (Upper triangular: U_row_ptr, U_col_idx, U_val)
    std::vector<size_t> U_row_ptr_;
    std::vector<size_t> U_col_idx_;
    std::vector<real_t> U_val_;
};

} // namespace bharatopt

#endif // BHARATOPT_SPARSE_LU_HPP
