#include <bharatopt/sparse_lu.hpp>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <iostream>

namespace bharatopt {

SparseLUBasisSolver::SparseLUBasisSolver(real_t pivot_tol)
    : pivot_tol_(pivot_tol) {}

CSRMatrix SparseLUBasisSolver::extract_sparse_basis(const StandardFormLP& lp, const Basis& basis) {
    size_t m = lp.num_rows;
    COOMatrix coo(m, m);

    for (size_t j = 0; j < m; ++j) {
        index_t bvar = basis.basic_vars[j];
        for (size_t i = 0; i < m; ++i) {
            real_t val = lp.A[i][bvar];
            if (std::abs(val) > DEFAULT_ZERO_TOLERANCE) {
                coo.add_entry(static_cast<index_t>(i), static_cast<index_t>(j), val);
            }
        }
    }

    return CSRMatrix::from_coo(coo);
}

bool SparseLUBasisSolver::factorize(const StandardFormLP& lp, const Basis& basis) {
    CSRMatrix B_sparse = extract_sparse_basis(lp, basis);
    return factorize_matrix(B_sparse);
}

bool SparseLUBasisSolver::factorize_matrix(const CSRMatrix& B_matrix) {
    m_ = B_matrix.rows();
    if (m_ == 0 || B_matrix.cols() != m_) {
        return false;
    }

    stats_ = SparseLUStats();
    stats_.matrix_dim = m_;
    stats_.original_basis_nnz = B_matrix.nnz();

    // Working dense representation for step-by-step LU decomposition with partial row pivoting
    std::vector<std::vector<real_t>> W(m_, std::vector<real_t>(m_, 0.0));
    for (size_t i = 0; i < m_; ++i) {
        size_t start = B_matrix.row_offsets()[i];
        size_t end = B_matrix.row_offsets()[i + 1];
        for (size_t p = start; p < end; ++p) {
            size_t col = B_matrix.col_indices()[p];
            W[i][col] = B_matrix.values()[p];
        }
    }

    P_row_.resize(m_);
    std::iota(P_row_.begin(), P_row_.end(), 0);
    P_inv_.resize(m_);

    stats_.pivot_min = BHARATOPT_INFINITY;
    stats_.pivot_max = 0.0;

    // Direct LU Decomposition with Row Partial Pivoting
    for (size_t k = 0; k < m_; ++k) {
        // Pivot Selection: find row i >= k with maximum absolute entry in column k
        size_t pivot_row = k;
        real_t max_val = std::abs(W[k][k]);

        for (size_t i = k + 1; i < m_; ++i) {
            real_t val = std::abs(W[i][k]);
            if (val > max_val) {
                max_val = val;
                pivot_row = i;
            }
        }

        if (max_val < pivot_tol_) {
            return false; // Singular or numerically unstable basis matrix
        }

        if (pivot_row != k) {
            std::swap(W[k], W[pivot_row]);
            std::swap(P_row_[k], P_row_[pivot_row]);
        }

        real_t pivot = W[k][k];
        real_t abs_pivot = std::abs(pivot);
        if (abs_pivot < stats_.pivot_min) stats_.pivot_min = abs_pivot;
        if (abs_pivot > stats_.pivot_max) stats_.pivot_max = abs_pivot;

        // Eliminate column entries below diagonal
        for (size_t i = k + 1; i < m_; ++i) {
            if (std::abs(W[i][k]) > DEFAULT_ZERO_TOLERANCE) {
                W[i][k] /= pivot;
                for (size_t j = k + 1; j < m_; ++j) {
                    if (std::abs(W[k][j]) > DEFAULT_ZERO_TOLERANCE) {
                        W[i][j] -= W[i][k] * W[k][j];
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < m_; ++i) {
        P_inv_[P_row_[i]] = i;
    }

    // Extract Sparse L (unit lower triangular) and Sparse U (upper triangular)
    L_row_ptr_.assign(m_ + 1, 0);
    L_col_idx_.clear();
    L_val_.clear();

    U_row_ptr_.assign(m_ + 1, 0);
    U_col_idx_.clear();
    U_val_.clear();

    for (size_t i = 0; i < m_; ++i) {
        // L factor: unit diagonal at j = i
        for (size_t j = 0; j < i; ++j) {
            if (std::abs(W[i][j]) > DEFAULT_ZERO_TOLERANCE) {
                L_col_idx_.push_back(j);
                L_val_.push_back(W[i][j]);
            }
        }
        L_col_idx_.push_back(i);
        L_val_.push_back(1.0); // Unit diagonal
        L_row_ptr_[i + 1] = L_col_idx_.size();

        // U factor: upper triangular entries for j >= i
        for (size_t j = i; j < m_; ++j) {
            if (std::abs(W[i][j]) > DEFAULT_ZERO_TOLERANCE) {
                U_col_idx_.push_back(j);
                U_val_.push_back(W[i][j]);
            }
        }
        U_row_ptr_[i + 1] = U_col_idx_.size();
    }

    // Compute Fill-in Metrics
    stats_.l_nnz = L_col_idx_.size();
    stats_.u_nnz = U_col_idx_.size();
    stats_.total_factor_nnz = stats_.l_nnz + stats_.u_nnz - m_; // Subtract duplicate unit diagonal count
    stats_.fill_in_nnz = static_cast<int64_t>(stats_.total_factor_nnz) - static_cast<int64_t>(stats_.original_basis_nnz);
    stats_.fill_in_ratio = static_cast<real_t>(stats_.total_factor_nnz) / std::max<size_t>(1, stats_.original_basis_nnz);

    return true;
}

bool SparseLUBasisSolver::solve_primal(const std::vector<real_t>& rhs, std::vector<real_t>& x) const {
    if (rhs.size() != m_) return false;
    x.resize(m_);

    // Step 1: Permute RHS according to row permutation vector P_row
    std::vector<real_t> b_perm(m_);
    for (size_t i = 0; i < m_; ++i) {
        b_perm[i] = rhs[P_row_[i]];
    }

    // Step 2: Sparse Forward Substitution for L y = P rhs
    std::vector<real_t> y(m_, 0.0);
    for (size_t i = 0; i < m_; ++i) {
        real_t sum = b_perm[i];
        size_t start = L_row_ptr_[i];
        size_t end = L_row_ptr_[i + 1];

        for (size_t p = start; p < end; ++p) {
            size_t j = L_col_idx_[p];
            if (j < i) {
                sum -= L_val_[p] * y[j];
            }
        }
        y[i] = sum; // Unit diagonal L_ii = 1
    }

    // Step 3: Sparse Backward Substitution for U x = y
    for (int i = static_cast<int>(m_) - 1; i >= 0; --i) {
        size_t row = static_cast<size_t>(i);
        real_t sum = y[row];
        real_t diag = 0.0;
        size_t start = U_row_ptr_[row];
        size_t end = U_row_ptr_[row + 1];

        for (size_t p = start; p < end; ++p) {
            size_t j = U_col_idx_[p];
            if (j == row) {
                diag = U_val_[p];
            } else if (j > row) {
                sum -= U_val_[p] * x[j];
            }
        }

        if (std::abs(diag) < DEFAULT_ZERO_TOLERANCE) {
            return false;
        }
        x[row] = sum / diag;
    }

    return true;
}

bool SparseLUBasisSolver::solve_dual(const std::vector<real_t>& rhs, std::vector<real_t>& y) const {
    if (rhs.size() != m_) return false;
    y.resize(m_);

    // Dual solve for B^T y = rhs => (P^T L U)^T y = rhs => U^T L^T P y = rhs
    // Step 1: Forward Substitution for U^T w = rhs
    std::vector<real_t> w(m_, 0.0);
    for (size_t i = 0; i < m_; ++i) {
        real_t sum = rhs[i];
        for (size_t k = 0; k < i; ++k) {
            // Find entry U[k][i]
            size_t start = U_row_ptr_[k];
            size_t end = U_row_ptr_[k + 1];
            for (size_t p = start; p < end; ++p) {
                if (U_col_idx_[p] == i) {
                    sum -= U_val_[p] * w[k];
                    break;
                }
            }
        }

        // Find diagonal entry U[i][i]
        real_t diag = 0.0;
        size_t start = U_row_ptr_[i];
        size_t end = U_row_ptr_[i + 1];
        for (size_t p = start; p < end; ++p) {
            if (U_col_idx_[p] == i) {
                diag = U_val_[p];
                break;
            }
        }

        if (std::abs(diag) < DEFAULT_ZERO_TOLERANCE) {
            return false;
        }
        w[i] = sum / diag;
    }

    // Step 2: Backward Substitution for L^T z = w (Unit diagonal)
    std::vector<real_t> z(m_, 0.0);
    for (int i = static_cast<int>(m_) - 1; i >= 0; --i) {
        size_t row = static_cast<size_t>(i);
        real_t sum = w[row];
        for (size_t k = row + 1; k < m_; ++k) {
            // Find entry L[k][row]
            size_t start = L_row_ptr_[k];
            size_t end = L_row_ptr_[k + 1];
            for (size_t p = start; p < end; ++p) {
                if (L_col_idx_[p] == row) {
                    sum -= L_val_[p] * z[k];
                    break;
                }
            }
        }
        z[row] = sum;
    }

    // Step 3: Unpermute dual solution y = P^T z => y[P_row_[i]] = z[i]
    for (size_t i = 0; i < m_; ++i) {
        y[P_row_[i]] = z[i];
    }

    return true;
}

} // namespace bharatopt
