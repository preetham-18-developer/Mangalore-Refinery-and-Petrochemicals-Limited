#include <bharatopt/sparse_matrix.hpp>
#include <algorithm>
#include <cmath>

namespace bharatopt {

// ============================================================================
// COOMatrix Implementation
// ============================================================================

COOMatrix::COOMatrix(size_t rows, size_t cols)
    : rows_(rows), cols_(cols) {}

void COOMatrix::add_entry(index_t row, index_t col, real_t val) {
    if (row < 0 || (rows_ > 0 && static_cast<size_t>(row) >= rows_)) {
        throw std::out_of_range("COO row index out of range: " + std::to_string(row));
    }
    if (col < 0 || (cols_ > 0 && static_cast<size_t>(col) >= cols_)) {
        throw std::out_of_range("COO col index out of range: " + std::to_string(col));
    }
    triplets_.push_back({row, col, val});
}

void COOMatrix::clear() {
    rows_ = 0;
    cols_ = 0;
    triplets_.clear();
}

COOMatrix COOMatrix::from_lp_model(const LPModel& model) {
    COOMatrix coo(model.num_constraints(), model.num_variables());
    const auto& constraints = model.constraints();
    for (size_t i = 0; i < constraints.size(); ++i) {
        index_t r = static_cast<index_t>(i);
        for (const auto& term : constraints[i].terms) {
            coo.add_entry(r, term.first, term.second);
        }
    }
    return coo;
}

// ============================================================================
// CSRMatrix Implementation
// ============================================================================

CSRMatrix::CSRMatrix(size_t rows, size_t cols,
                     std::vector<index_t> row_offsets,
                     std::vector<index_t> col_indices,
                     std::vector<real_t> values)
    : rows_(rows), cols_(cols),
      row_offsets_(std::move(row_offsets)),
      col_indices_(std::move(col_indices)),
      values_(std::move(values)) {
    if (!is_valid()) {
        throw std::invalid_argument("Attempted to construct invalid CSRMatrix.");
    }
}

CSRMatrix CSRMatrix::from_coo(const COOMatrix& coo) {
    size_t m = coo.rows();
    size_t n = coo.cols();
    auto triplets = coo.triplets();

    // Sort triplets by (row, col)
    std::sort(triplets.begin(), triplets.end(), [](const COOTriplet& a, const COOTriplet& b) {
        if (a.row != b.row) return a.row < b.row;
        return a.col < b.col;
    });

    // Accumulate duplicates and filter explicit zeros
    std::vector<COOTriplet> clean_triplets;
    clean_triplets.reserve(triplets.size());

    for (const auto& t : triplets) {
        if (t.row < 0 || (m > 0 && static_cast<size_t>(t.row) >= m) ||
            t.col < 0 || (n > 0 && static_cast<size_t>(t.col) >= n)) {
            throw std::out_of_range("COO triplet index out of range during CSR conversion.");
        }

        if (!clean_triplets.empty() &&
            clean_triplets.back().row == t.row &&
            clean_triplets.back().col == t.col) {
            clean_triplets.back().val += t.val;
        } else {
            clean_triplets.push_back(t);
        }
    }

    // Build CSR arrays with zero-pruning
    std::vector<index_t> row_offsets(m + 1, 0);
    std::vector<index_t> col_indices;
    std::vector<real_t> values;

    col_indices.reserve(clean_triplets.size());
    values.reserve(clean_triplets.size());

    for (const auto& t : clean_triplets) {
        if (t.val != 0.0) {
            col_indices.push_back(t.col);
            values.push_back(t.val);
            row_offsets[static_cast<size_t>(t.row) + 1]++;
        }
    }

    // Cumulative sum for row_offsets
    for (size_t i = 0; i < m; ++i) {
        row_offsets[i + 1] += row_offsets[i];
    }

    return CSRMatrix(m, n, std::move(row_offsets), std::move(col_indices), std::move(values));
}

CSRMatrix CSRMatrix::from_csc(const CSCMatrix& csc) {
    COOMatrix coo(csc.rows(), csc.cols());
    for (size_t j = 0; j < csc.cols(); ++j) {
        index_t col = static_cast<index_t>(j);
        index_t start = csc.col_offsets()[j];
        index_t end = csc.col_offsets()[j + 1];
        for (index_t k = start; k < end; ++k) {
            coo.add_entry(csc.row_indices()[static_cast<size_t>(k)], col, csc.values()[static_cast<size_t>(k)]);
        }
    }
    return CSRMatrix::from_coo(coo);
}

real_t CSRMatrix::get(index_t row, index_t col) const {
    if (row < 0 || static_cast<size_t>(row) >= rows_ ||
        col < 0 || static_cast<size_t>(col) >= cols_) {
        throw std::out_of_range("CSR index out of range.");
    }

    index_t start = row_offsets_[static_cast<size_t>(row)];
    index_t end = row_offsets_[static_cast<size_t>(row) + 1];

    for (index_t k = start; k < end; ++k) {
        if (col_indices_[static_cast<size_t>(k)] == col) {
            return values_[static_cast<size_t>(k)];
        }
    }
    return 0.0;
}

std::vector<real_t> CSRMatrix::multiply(const std::vector<real_t>& x) const {
    if (x.size() != cols_) {
        throw std::invalid_argument("SpMV dimension mismatch: x size " + std::to_string(x.size()) +
                                   " != cols " + std::to_string(cols_));
    }
    std::vector<real_t> y(rows_, 0.0);
    multiply(x.data(), y.data());
    return y;
}

void CSRMatrix::multiply(const real_t* x, real_t* y) const {
    for (size_t i = 0; i < rows_; ++i) {
        real_t sum = 0.0;
        index_t start = row_offsets_[i];
        index_t end = row_offsets_[i + 1];
        for (index_t k = start; k < end; ++k) {
            sum += values_[static_cast<size_t>(k)] * x[static_cast<size_t>(col_indices_[static_cast<size_t>(k)])];
        }
        y[i] = sum;
    }
}

std::vector<real_t> CSRMatrix::multiply_transpose(const std::vector<real_t>& x) const {
    if (x.size() != rows_) {
        throw std::invalid_argument("Transpose SpMV dimension mismatch: x size " + std::to_string(x.size()) +
                                   " != rows " + std::to_string(rows_));
    }
    std::vector<real_t> y(cols_, 0.0);
    multiply_transpose(x.data(), y.data());
    return y;
}

void CSRMatrix::multiply_transpose(const real_t* x, real_t* y) const {
    std::fill(y, y + cols_, 0.0);
    for (size_t i = 0; i < rows_; ++i) {
        real_t xi = x[i];
        index_t start = row_offsets_[i];
        index_t end = row_offsets_[i + 1];
        for (index_t k = start; k < end; ++k) {
            y[static_cast<size_t>(col_indices_[static_cast<size_t>(k)])] += values_[static_cast<size_t>(k)] * xi;
        }
    }
}

bool CSRMatrix::is_valid() const {
    if (row_offsets_.size() != rows_ + 1) return false;
    if (row_offsets_[0] != 0) return false;
    if (row_offsets_.back() != static_cast<index_t>(values_.size())) return false;
    if (col_indices_.size() != values_.size()) return false;

    for (size_t i = 0; i < rows_; ++i) {
        index_t start = row_offsets_[i];
        index_t end = row_offsets_[i + 1];
        if (start > end) return false;

        index_t prev_col = -1;
        for (index_t k = start; k < end; ++k) {
            index_t c = col_indices_[static_cast<size_t>(k)];
            if (c < 0 || static_cast<size_t>(c) >= cols_) return false;
            if (c <= prev_col) return false; // Strictly ascending order
            prev_col = c;
        }
    }
    return true;
}

// ============================================================================
// CSCMatrix Implementation
// ============================================================================

CSCMatrix::CSCMatrix(size_t rows, size_t cols,
                     std::vector<index_t> col_offsets,
                     std::vector<index_t> row_indices,
                     std::vector<real_t> values)
    : rows_(rows), cols_(cols),
      col_offsets_(std::move(col_offsets)),
      row_indices_(std::move(row_indices)),
      values_(std::move(values)) {
    if (!is_valid()) {
        throw std::invalid_argument("Attempted to construct invalid CSCMatrix.");
    }
}

CSCMatrix CSCMatrix::from_coo(const COOMatrix& coo) {
    size_t m = coo.rows();
    size_t n = coo.cols();
    auto triplets = coo.triplets();

    // Sort triplets by (col, row)
    std::sort(triplets.begin(), triplets.end(), [](const COOTriplet& a, const COOTriplet& b) {
        if (a.col != b.col) return a.col < b.col;
        return a.row < b.row;
    });

    // Accumulate duplicates
    std::vector<COOTriplet> clean_triplets;
    clean_triplets.reserve(triplets.size());

    for (const auto& t : triplets) {
        if (t.row < 0 || (m > 0 && static_cast<size_t>(t.row) >= m) ||
            t.col < 0 || (n > 0 && static_cast<size_t>(t.col) >= n)) {
            throw std::out_of_range("COO triplet index out of range during CSC conversion.");
        }

        if (!clean_triplets.empty() &&
            clean_triplets.back().row == t.row &&
            clean_triplets.back().col == t.col) {
            clean_triplets.back().val += t.val;
        } else {
            clean_triplets.push_back(t);
        }
    }

    // Build CSC arrays with zero-pruning
    std::vector<index_t> col_offsets(n + 1, 0);
    std::vector<index_t> row_indices;
    std::vector<real_t> values;

    row_indices.reserve(clean_triplets.size());
    values.reserve(clean_triplets.size());

    for (const auto& t : clean_triplets) {
        if (t.val != 0.0) {
            row_indices.push_back(t.row);
            values.push_back(t.val);
            col_offsets[static_cast<size_t>(t.col) + 1]++;
        }
    }

    // Cumulative sum
    for (size_t j = 0; j < n; ++j) {
        col_offsets[j + 1] += col_offsets[j];
    }

    return CSCMatrix(m, n, std::move(col_offsets), std::move(row_indices), std::move(values));
}

CSCMatrix CSCMatrix::from_csr(const CSRMatrix& csr) {
    COOMatrix coo(csr.rows(), csr.cols());
    for (size_t i = 0; i < csr.rows(); ++i) {
        index_t row = static_cast<index_t>(i);
        index_t start = csr.row_offsets()[i];
        index_t end = csr.row_offsets()[i + 1];
        for (index_t k = start; k < end; ++k) {
            coo.add_entry(row, csr.col_indices()[static_cast<size_t>(k)], csr.values()[static_cast<size_t>(k)]);
        }
    }
    return CSCMatrix::from_coo(coo);
}

real_t CSCMatrix::get(index_t row, index_t col) const {
    if (row < 0 || static_cast<size_t>(row) >= rows_ ||
        col < 0 || static_cast<size_t>(col) >= cols_) {
        throw std::out_of_range("CSC index out of range.");
    }

    index_t start = col_offsets_[static_cast<size_t>(col)];
    index_t end = col_offsets_[static_cast<size_t>(col) + 1];

    for (index_t k = start; k < end; ++k) {
        if (row_indices_[static_cast<size_t>(k)] == row) {
            return values_[static_cast<size_t>(k)];
        }
    }
    return 0.0;
}

std::vector<real_t> CSCMatrix::multiply(const std::vector<real_t>& x) const {
    if (x.size() != cols_) {
        throw std::invalid_argument("SpMV dimension mismatch: x size " + std::to_string(x.size()) +
                                   " != cols " + std::to_string(cols_));
    }
    std::vector<real_t> y(rows_, 0.0);
    multiply(x.data(), y.data());
    return y;
}

void CSCMatrix::multiply(const real_t* x, real_t* y) const {
    std::fill(y, y + rows_, 0.0);
    for (size_t j = 0; j < cols_; ++j) {
        real_t xj = x[j];
        index_t start = col_offsets_[j];
        index_t end = col_offsets_[j + 1];
        for (index_t k = start; k < end; ++k) {
            y[static_cast<size_t>(row_indices_[static_cast<size_t>(k)])] += values_[static_cast<size_t>(k)] * xj;
        }
    }
}

std::vector<real_t> CSCMatrix::multiply_transpose(const std::vector<real_t>& x) const {
    if (x.size() != rows_) {
        throw std::invalid_argument("Transpose SpMV dimension mismatch: x size " + std::to_string(x.size()) +
                                   " != rows " + std::to_string(rows_));
    }
    std::vector<real_t> y(cols_, 0.0);
    multiply_transpose(x.data(), y.data());
    return y;
}

void CSCMatrix::multiply_transpose(const real_t* x, real_t* y) const {
    for (size_t j = 0; j < cols_; ++j) {
        real_t sum = 0.0;
        index_t start = col_offsets_[j];
        index_t end = col_offsets_[j + 1];
        for (index_t k = start; k < end; ++k) {
            sum += values_[static_cast<size_t>(k)] * x[static_cast<size_t>(row_indices_[static_cast<size_t>(k)])];
        }
        y[j] = sum;
    }
}

bool CSCMatrix::is_valid() const {
    if (col_offsets_.size() != cols_ + 1) return false;
    if (col_offsets_[0] != 0) return false;
    if (col_offsets_.back() != static_cast<index_t>(values_.size())) return false;
    if (row_indices_.size() != values_.size()) return false;

    for (size_t j = 0; j < cols_; ++j) {
        index_t start = col_offsets_[j];
        index_t end = col_offsets_[j + 1];
        if (start > end) return false;

        index_t prev_row = -1;
        for (index_t k = start; k < end; ++k) {
            index_t r = row_indices_[static_cast<size_t>(k)];
            if (r < 0 || static_cast<size_t>(r) >= rows_) return false;
            if (r <= prev_row) return false; // Strictly ascending order
            prev_row = r;
        }
    }
    return true;
}

} // namespace bharatopt
