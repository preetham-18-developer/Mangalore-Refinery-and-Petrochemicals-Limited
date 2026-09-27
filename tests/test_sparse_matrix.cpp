#include "test_harness.hpp"
#include <bharatopt/sparse_matrix.hpp>
#include <cmath>
#include <vector>
#include <stdexcept>

using namespace bharatopt;

// Dense Reference Matrix for numerical correctness oracle
class DenseReferenceMatrix {
public:
    DenseReferenceMatrix(size_t rows, size_t cols)
        : rows_(rows), cols_(cols), data_(rows * cols, 0.0) {}

    void set(size_t r, size_t c, real_t val) {
        data_[r * cols_ + c] = val;
    }

    std::vector<real_t> multiply(const std::vector<real_t>& x) const {
        std::vector<real_t> y(rows_, 0.0);
        for (size_t r = 0; r < rows_; ++r) {
            for (size_t c = 0; c < cols_; ++c) {
                y[r] += data_[r * cols_ + c] * x[c];
            }
        }
        return y;
    }

    std::vector<real_t> multiply_transpose(const std::vector<real_t>& x) const {
        std::vector<real_t> y(cols_, 0.0);
        for (size_t r = 0; r < rows_; ++r) {
            for (size_t c = 0; c < cols_; ++c) {
                y[c] += data_[r * cols_ + c] * x[r];
            }
        }
        return y;
    }

private:
    size_t rows_;
    size_t cols_;
    std::vector<real_t> data_;
};

// ============================================================================
// DETERMINISTIC TEST MATRICES
// ============================================================================

TEST_CASE(Sparse_01_EmptyMatrix) {
    COOMatrix coo(0, 0);
    CSRMatrix csr = CSRMatrix::from_coo(coo);
    CSCMatrix csc = CSCMatrix::from_coo(coo);

    EXPECT_EQ(csr.rows(), static_cast<size_t>(0));
    EXPECT_EQ(csr.cols(), static_cast<size_t>(0));
    EXPECT_EQ(csr.nnz(), static_cast<size_t>(0));
    EXPECT_EQ(csc.nnz(), static_cast<size_t>(0));
    EXPECT_TRUE(csr.is_valid());
    EXPECT_TRUE(csc.is_valid());
}

TEST_CASE(Sparse_02_OneByOneMatrix) {
    COOMatrix coo(1, 1);
    coo.add_entry(0, 0, 42.0);

    CSRMatrix csr = CSRMatrix::from_coo(coo);
    CSCMatrix csc = CSCMatrix::from_coo(coo);

    EXPECT_NEAR(csr.get(0, 0), 42.0, 1e-12);
    EXPECT_NEAR(csc.get(0, 0), 42.0, 1e-12);

    std::vector<real_t> x = {2.0};
    std::vector<real_t> y_csr = csr.multiply(x);
    std::vector<real_t> y_csc = csc.multiply(x);

    EXPECT_NEAR(y_csr[0], 84.0, 1e-12);
    EXPECT_NEAR(y_csc[0], 84.0, 1e-12);
}

TEST_CASE(Sparse_03_SingleRowMatrix) {
    COOMatrix coo(1, 3);
    coo.add_entry(0, 0, 1.0);
    coo.add_entry(0, 2, 3.0);

    CSRMatrix csr = CSRMatrix::from_coo(coo);
    CSCMatrix csc = CSCMatrix::from_coo(coo);

    std::vector<real_t> x = {10.0, 20.0, 30.0};
    // y = A * x = 1*10 + 0*20 + 3*30 = 100
    std::vector<real_t> y = csr.multiply(x);
    EXPECT_NEAR(y[0], 100.0, 1e-12);

    // y_t = A^T * [5] = [5, 0, 15]
    std::vector<real_t> y_t = csr.multiply_transpose({5.0});
    EXPECT_NEAR(y_t[0], 5.0, 1e-12);
    EXPECT_NEAR(y_t[1], 0.0, 1e-12);
    EXPECT_NEAR(y_t[2], 15.0, 1e-12);
}

TEST_CASE(Sparse_04_SingleColumnMatrix) {
    COOMatrix coo(3, 1);
    coo.add_entry(0, 0, 2.0);
    coo.add_entry(2, 0, 5.0);

    CSRMatrix csr = CSRMatrix::from_coo(coo);
    CSCMatrix csc = CSCMatrix::from_coo(coo);

    std::vector<real_t> x = {4.0};
    std::vector<real_t> y = csc.multiply(x);
    EXPECT_NEAR(y[0], 8.0, 1e-12);
    EXPECT_NEAR(y[1], 0.0, 1e-12);
    EXPECT_NEAR(y[2], 20.0, 1e-12);
}

TEST_CASE(Sparse_05_DiagonalMatrix) {
    COOMatrix coo(4, 4);
    for (int i = 0; i < 4; ++i) {
        coo.add_entry(i, i, static_cast<real_t>(i + 1));
    }

    CSRMatrix csr = CSRMatrix::from_coo(coo);
    std::vector<real_t> x = {1.0, 1.0, 1.0, 1.0};
    std::vector<real_t> y = csr.multiply(x);

    for (int i = 0; i < 4; ++i) {
        EXPECT_NEAR(y[i], static_cast<real_t>(i + 1), 1e-12);
    }
}

TEST_CASE(Sparse_06_IdentityMatrix) {
    COOMatrix coo(5, 5);
    for (int i = 0; i < 5; ++i) {
        coo.add_entry(i, i, 1.0);
    }

    CSRMatrix csr = CSRMatrix::from_coo(coo);
    std::vector<real_t> x = {1.5, 2.5, 3.5, 4.5, 5.5};
    std::vector<real_t> y = csr.multiply(x);

    for (int i = 0; i < 5; ++i) {
        EXPECT_NEAR(y[i], x[i], 1e-12);
    }
}

TEST_CASE(Sparse_07_DenseSmallMatrix) {
    // 2x3 dense matrix represented as COO
    // [ 1.0  2.0  3.0 ]
    // [ 4.0  5.0  6.0 ]
    COOMatrix coo(2, 3);
    DenseReferenceMatrix ref(2, 3);

    real_t vals[2][3] = {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 3; ++c) {
            coo.add_entry(r, c, vals[r][c]);
            ref.set(r, c, vals[r][c]);
        }
    }

    CSRMatrix csr = CSRMatrix::from_coo(coo);
    CSCMatrix csc = CSCMatrix::from_coo(coo);

    std::vector<real_t> x = {0.5, -1.0, 2.0};
    std::vector<real_t> ref_y = ref.multiply(x);
    std::vector<real_t> csr_y = csr.multiply(x);
    std::vector<real_t> csc_y = csc.multiply(x);

    for (size_t i = 0; i < 2; ++i) {
        EXPECT_NEAR(csr_y[i], ref_y[i], 1e-12);
        EXPECT_NEAR(csc_y[i], ref_y[i], 1e-12);
    }

    std::vector<real_t> xt = {3.0, -2.0};
    std::vector<real_t> ref_yt = ref.multiply_transpose(xt);
    std::vector<real_t> csr_yt = csr.multiply_transpose(xt);
    std::vector<real_t> csc_yt = csc.multiply_transpose(xt);

    for (size_t j = 0; j < 3; ++j) {
        EXPECT_NEAR(csr_yt[j], ref_yt[j], 1e-12);
        EXPECT_NEAR(csc_yt[j], ref_yt[j], 1e-12);
    }
}

TEST_CASE(Sparse_08_HighlySparseMatrix) {
    COOMatrix coo(100, 100);
    coo.add_entry(0, 99, 5.0);
    coo.add_entry(50, 25, -3.0);
    coo.add_entry(99, 0, 7.0);

    CSRMatrix csr = CSRMatrix::from_coo(coo);
    EXPECT_EQ(csr.nnz(), static_cast<size_t>(3));
    EXPECT_NEAR(csr.get(0, 99), 5.0, 1e-12);
    EXPECT_NEAR(csr.get(50, 25), -3.0, 1e-12);
    EXPECT_NEAR(csr.get(99, 0), 7.0, 1e-12);
}

TEST_CASE(Sparse_09_ZeroValuePolicyPruning) {
    COOMatrix coo(2, 2);
    coo.add_entry(0, 0, 1.0);
    coo.add_entry(0, 1, 0.0); // Explicit zero
    coo.add_entry(1, 0, 0.0); // Explicit zero
    coo.add_entry(1, 1, 2.0);

    CSRMatrix csr = CSRMatrix::from_coo(coo);
    EXPECT_EQ(csr.nnz(), static_cast<size_t>(2)); // Explicit zeros pruned
    EXPECT_NEAR(csr.get(0, 0), 1.0, 1e-12);
    EXPECT_NEAR(csr.get(0, 1), 0.0, 1e-12);
    EXPECT_NEAR(csr.get(1, 1), 2.0, 1e-12);
}

TEST_CASE(Sparse_10_NegativeValues) {
    COOMatrix coo(2, 2);
    coo.add_entry(0, 0, -5.5);
    coo.add_entry(1, 1, -10.2);

    CSRMatrix csr = CSRMatrix::from_coo(coo);
    std::vector<real_t> x = {2.0, 3.0};
    std::vector<real_t> y = csr.multiply(x);

    EXPECT_NEAR(y[0], -11.0, 1e-12);
    EXPECT_NEAR(y[1], -30.6, 1e-12);
}

TEST_CASE(Sparse_11_DuplicateCOOAccumulation) {
    COOMatrix coo(2, 2);
    // Duplicate triplets at (0, 0)
    coo.add_entry(0, 0, 2.0);
    coo.add_entry(0, 0, 3.0);
    coo.add_entry(0, 0, -1.0); // Net sum = 4.0

    CSRMatrix csr = CSRMatrix::from_coo(coo);
    EXPECT_EQ(csr.nnz(), static_cast<size_t>(1));
    EXPECT_NEAR(csr.get(0, 0), 4.0, 1e-12);
}

TEST_CASE(Sparse_12_UnsortedCOOEntries) {
    COOMatrix coo(3, 3);
    // Insert out of order
    coo.add_entry(2, 2, 9.0);
    coo.add_entry(0, 1, 2.0);
    coo.add_entry(1, 0, 4.0);
    coo.add_entry(0, 0, 1.0);

    CSRMatrix csr = CSRMatrix::from_coo(coo);
    CSCMatrix csc = CSCMatrix::from_coo(coo);

    EXPECT_TRUE(csr.is_valid());
    EXPECT_TRUE(csc.is_valid());

    // Verify canonical sorting in CSR
    EXPECT_EQ(csr.col_indices()[0], 0);
    EXPECT_EQ(csr.col_indices()[1], 1);
}

TEST_CASE(Sparse_13_EmptyRows) {
    COOMatrix coo(4, 2);
    coo.add_entry(1, 0, 3.0);
    // Rows 0, 2, 3 are empty

    CSRMatrix csr = CSRMatrix::from_coo(coo);
    EXPECT_EQ(csr.nnz(), static_cast<size_t>(1));
    EXPECT_NEAR(csr.get(1, 0), 3.0, 1e-12);
    EXPECT_NEAR(csr.get(0, 0), 0.0, 1e-12);

    std::vector<real_t> y = csr.multiply({2.0, 5.0});
    EXPECT_NEAR(y[0], 0.0, 1e-12);
    EXPECT_NEAR(y[1], 6.0, 1e-12);
    EXPECT_NEAR(y[2], 0.0, 1e-12);
    EXPECT_NEAR(y[3], 0.0, 1e-12);
}

TEST_CASE(Sparse_14_EmptyColumns) {
    COOMatrix coo(2, 4);
    coo.add_entry(0, 1, 4.0);
    // Cols 0, 2, 3 are empty

    CSCMatrix csc = CSCMatrix::from_coo(coo);
    EXPECT_EQ(csc.nnz(), static_cast<size_t>(1));

    std::vector<real_t> yt = csc.multiply_transpose({3.0, 1.0});
    EXPECT_NEAR(yt[0], 0.0, 1e-12);
    EXPECT_NEAR(yt[1], 12.0, 1e-12);
    EXPECT_NEAR(yt[2], 0.0, 1e-12);
    EXPECT_NEAR(yt[3], 0.0, 1e-12);
}

TEST_CASE(Sparse_15_RectangularMatrix) {
    // 3 rows x 5 cols
    COOMatrix coo(3, 5);
    coo.add_entry(0, 0, 1.0);
    coo.add_entry(0, 4, 2.0);
    coo.add_entry(1, 2, 3.0);
    coo.add_entry(2, 1, 4.0);
    coo.add_entry(2, 3, 5.0);

    CSRMatrix csr = CSRMatrix::from_coo(coo);
    CSCMatrix csc = CSCMatrix::from_coo(coo);

    EXPECT_EQ(csr.rows(), static_cast<size_t>(3));
    EXPECT_EQ(csr.cols(), static_cast<size_t>(5));

    std::vector<real_t> x = {1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<real_t> y_csr = csr.multiply(x);
    std::vector<real_t> y_csc = csc.multiply(x);

    // y[0] = 1*1 + 2*5 = 11
    // y[1] = 3*3 = 9
    // y[2] = 4*2 + 5*4 = 28
    EXPECT_NEAR(y_csr[0], 11.0, 1e-12);
    EXPECT_NEAR(y_csr[1], 9.0, 1e-12);
    EXPECT_NEAR(y_csr[2], 28.0, 1e-12);

    for (size_t i = 0; i < 3; ++i) {
        EXPECT_NEAR(y_csr[i], y_csc[i], 1e-12);
    }
}

TEST_CASE(Sparse_16_LargeSyntheticMatrix) {
    // 100,000 x 100,000 matrix with 500,000 NNZ (5 entries per row)
    constexpr size_t N = 100000;
    COOMatrix coo(N, N);

    for (size_t i = 0; i < N; ++i) {
        index_t r = static_cast<index_t>(i);
        coo.add_entry(r, r, 2.0); // Diagonal
        coo.add_entry(r, static_cast<index_t>((i + 1) % N), -0.5);
        coo.add_entry(r, static_cast<index_t>((i + 2) % N), -0.25);
    }

    CSRMatrix csr = CSRMatrix::from_coo(coo);
    EXPECT_EQ(csr.rows(), N);
    EXPECT_EQ(csr.cols(), N);
    EXPECT_EQ(csr.nnz(), N * 3);
    EXPECT_TRUE(csr.is_valid());

    std::vector<real_t> x(N, 1.0);
    std::vector<real_t> y = csr.multiply(x);

    // y[i] = 2.0*1.0 - 0.5*1.0 - 0.25*1.0 = 1.25
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_NEAR(y[i], 1.25, 1e-12);
    }
}

TEST_CASE(Sparse_17_FormatConversions) {
    COOMatrix coo(3, 3);
    coo.add_entry(0, 0, 1.0);
    coo.add_entry(0, 2, 2.0);
    coo.add_entry(1, 1, 3.0);
    coo.add_entry(2, 0, 4.0);

    CSRMatrix csr_from_coo = CSRMatrix::from_coo(coo);
    CSCMatrix csc_from_coo = CSCMatrix::from_coo(coo);

    CSRMatrix csr_from_csc = CSRMatrix::from_csc(csc_from_coo);
    CSCMatrix csc_from_csr = CSCMatrix::from_csr(csr_from_coo);

    EXPECT_EQ(csr_from_coo.nnz(), csr_from_csc.nnz());
    EXPECT_EQ(csc_from_coo.nnz(), csc_from_csr.nnz());

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            EXPECT_NEAR(csr_from_coo.get(r, c), csr_from_csc.get(r, c), 1e-12);
            EXPECT_NEAR(csc_from_coo.get(r, c), csc_from_csr.get(r, c), 1e-12);
        }
    }
}

TEST_CASE(Sparse_18_DimensionMismatchErrorHandling) {
    COOMatrix coo(2, 3);
    coo.add_entry(0, 0, 1.0);
    CSRMatrix csr = CSRMatrix::from_coo(coo);

    // x size 2 instead of 3 for SpMV (cols = 3)
    bool caught_spmv = false;
    try {
        (void)csr.multiply({1.0, 2.0});
    } catch (const std::invalid_argument&) {
        caught_spmv = true;
    }
    EXPECT_TRUE(caught_spmv);

    // x size 3 instead of 2 for Transpose SpMV (rows = 2)
    bool caught_t_spmv = false;
    try {
        (void)csr.multiply_transpose({1.0, 2.0, 3.0});
    } catch (const std::invalid_argument&) {
        caught_t_spmv = true;
    }
    EXPECT_TRUE(caught_t_spmv);
}
