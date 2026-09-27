#ifndef BHARATOPT_SPARSE_MATRIX_HPP
#define BHARATOPT_SPARSE_MATRIX_HPP

#include <vector>
#include <string>
#include <cstddef>
#include <stdexcept>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>

namespace bharatopt {

class CSRMatrix;
class CSCMatrix;

struct COOTriplet {
    index_t row{0};
    index_t col{0};
    real_t val{0.0};
};

class COOMatrix {
public:
    COOMatrix(size_t rows = 0, size_t cols = 0);

    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }
    size_t nnz() const { return triplets_.size(); }

    void add_entry(index_t row, index_t col, real_t val);
    void clear();

    const std::vector<COOTriplet>& triplets() const { return triplets_; }

    static COOMatrix from_lp_model(const LPModel& model);

private:
    size_t rows_{0};
    size_t cols_{0};
    std::vector<COOTriplet> triplets_;
};

class CSRMatrix {
public:
    CSRMatrix() = default;
    CSRMatrix(size_t rows, size_t cols,
              std::vector<index_t> row_offsets,
              std::vector<index_t> col_indices,
              std::vector<real_t> values);

    static CSRMatrix from_coo(const COOMatrix& coo);
    static CSRMatrix from_csc(const CSCMatrix& csc);

    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }
    size_t nnz() const { return values_.size(); }

    const std::vector<index_t>& row_offsets() const { return row_offsets_; }
    const std::vector<index_t>& col_indices() const { return col_indices_; }
    const std::vector<real_t>& values() const { return values_; }

    real_t get(index_t row, index_t col) const;

    // SpMV: y = A * x (x: cols -> y: rows)
    std::vector<real_t> multiply(const std::vector<real_t>& x) const;
    void multiply(const real_t* x, real_t* y) const;

    // Transpose SpMV: y = A^T * x (x: rows -> y: cols)
    std::vector<real_t> multiply_transpose(const std::vector<real_t>& x) const;
    void multiply_transpose(const real_t* x, real_t* y) const;

    bool is_valid() const;

private:
    size_t rows_{0};
    size_t cols_{0};
    std::vector<index_t> row_offsets_;
    std::vector<index_t> col_indices_;
    std::vector<real_t> values_;
};

class CSCMatrix {
public:
    CSCMatrix() = default;
    CSCMatrix(size_t rows, size_t cols,
              std::vector<index_t> col_offsets,
              std::vector<index_t> row_indices,
              std::vector<real_t> values);

    static CSCMatrix from_coo(const COOMatrix& coo);
    static CSCMatrix from_csr(const CSRMatrix& csr);

    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }
    size_t nnz() const { return values_.size(); }

    const std::vector<index_t>& col_offsets() const { return col_offsets_; }
    const std::vector<index_t>& row_indices() const { return row_indices_; }
    const std::vector<real_t>& values() const { return values_; }

    real_t get(index_t row, index_t col) const;

    // SpMV: y = A * x (x: cols -> y: rows)
    std::vector<real_t> multiply(const std::vector<real_t>& x) const;
    void multiply(const real_t* x, real_t* y) const;

    // Transpose SpMV: y = A^T * x (x: rows -> y: cols)
    std::vector<real_t> multiply_transpose(const std::vector<real_t>& x) const;
    void multiply_transpose(const real_t* x, real_t* y) const;

    bool is_valid() const;

private:
    size_t rows_{0};
    size_t cols_{0};
    std::vector<index_t> col_offsets_;
    std::vector<index_t> row_indices_;
    std::vector<real_t> values_;
};

} // namespace bharatopt

#endif // BHARATOPT_SPARSE_MATRIX_HPP
