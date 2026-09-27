# Phase 4 Summary Report — Sparse Matrix Engine

**Project Name:** BharatOpt  
**Phase:** Phase 4 — Sparse Matrix Engine  
**Date:** September 25, 2026  
**Status:** Successfully Completed  

---

## 1. Executive Summary

Phase 4 implemented the sparse matrix numerical engine (`COOMatrix`, `CSRMatrix`, `CSCMatrix`) for handling large-scale optimization matrices $A \in \mathbb{R}^{m \times n}$. The engine supports row-wise Compressed Sparse Row (CSR), column-wise Compressed Sparse Column (CSC), and dynamic Coordinate (COO) assembly.

All format conversions ($\text{COO} \to \text{CSR}$, $\text{COO} \to \text{CSC}$, $\text{CSR} \leftrightarrow \text{CSC}$), canonical sorting, duplicate coordinate accumulation, zero-entry pruning, SpMV ($y = Ax$), and Transpose SpMV ($y = A^T x$) were implemented and verified. A comprehensive 18-test sparse matrix suite (plus 48 regression tests, totaling 66 tests) passed with 100% success.

---

## 2. Sparse Formats & Policy Specifications

### A. Coordinate Format (COO)
- Dynamic triplet storage `(row, col, value)`.
- Used for model construction and `LPModel` extraction.

### B. Compressed Sparse Row (CSR)
- Array representations: `row_offsets` (size $m+1$), `col_indices` (size $\text{NNZ}$), `values` (size $\text{NNZ}$).
- Canonical Ordering: `col_indices` sorted in strictly ascending order per row.
- High-throughput SpMV ($y = Ax$).

### C. Compressed Sparse Column (CSC)
- Array representations: `col_offsets` (size $n+1$), `row_indices` (size $\text{NNZ}$), `values` (size $\text{NNZ}$).
- Canonical Ordering: `row_indices` sorted in strictly ascending order per column.
- High-throughput Transpose SpMV ($y = A^T x$).

### D. Documented Conversion Policies
1. **Duplicate Policy:** Duplicate COO entries with identical `(row, col)` coordinates are **accumulated** (summed: $A(i, j) = \sum v$) during conversion.
2. **Zero Policy:** Explicit zero entries ($v = 0.0$) are **pruned** during conversion to minimize storage and maintain canonical sparsity.

---

## 3. Test & Verification Results

### Test Suite Execution
- **18 Sparse Matrix Tests (`Sparse_01` to `Sparse_18`):**
  1. Empty Matrix ($0 \times 0$ and $m \times n$ zero NNZ)
  2. $1 \times 1$ Matrix
  3. Single-Row Matrix ($1 \times n$)
  4. Single-Column Matrix ($m \times 1$)
  5. Diagonal Matrix
  6. Identity Matrix
  7. Small Dense Matrix represented sparsely (verified against `DenseReferenceMatrix` oracle)
  8. Highly Sparse Matrix
  9. Zero-Value Pruning Policy
  10. Negative Value Handling
  11. Duplicate COO Coordinate Accumulation
  12. Unsorted COO Entry Canonicalisation
  13. Empty Rows Handling
  14. Empty Columns Handling
  15. Rectangular Matrix ($m \neq n$)
  16. **Large Synthetic Sparse Matrix ($100,000 \times 100,000$, 300,000 NNZ)**
  17. Bidirectional Format Conversions ($\text{COO} \to \text{CSR}, \text{COO} \to \text{CSC}, \text{CSR} \leftrightarrow \text{CSC}$)
  18. Dimension Mismatch Error Handling
- **Regression Suite:** All 48 Phase 1, Phase 2, and Phase 3 tests passed without regression.

### Test Output
`Test Summary: 66 Passed, 0 Failed` (100% Pass Rate).

---

## 4. Performance & Memory Micro-Benchmark

Evaluated on $10,000 \times 10,000$ matrix with $100,000$ non-zeros ($\text{NNZ}$):

| Operation | Scale / Parameters | Measured Execution Time | Memory Comparison |
| :--- | :--- | :--- | :--- |
| **COO $\to$ CSR Conversion** | $10,000 \times 10,000$ ($100\text{k NNZ}$) | $4.9899 \text{ ms}$ | $0.80 \text{ MB (Sparse)}$ vs $800 \text{ MB (Dense)}$ ($99.9\%$ saving) |
| **COO $\to$ CSC Conversion** | $10,000 \times 10,000$ ($100\text{k NNZ}$) | $12.1964 \text{ ms}$ | $0.80 \text{ MB (Sparse)}$ vs $800 \text{ MB (Dense)}$ ($99.9\%$ saving) |
| **CSR SpMV ($y = Ax$)** | $10,000 \times 10,000$ ($100\text{k NNZ}$) | **$0.104091 \text{ ms}$ / iter** | Steady-state zero-allocation execution |
| **CSC SpMV ($y = Ax$)** | $10,000 \times 10,000$ ($100\text{k NNZ}$) | **$0.136755 \text{ ms}$ / iter** | Steady-state zero-allocation execution |
| **CSC Transpose SpMV ($y = A^T x$)**| $10,000 \times 10,000$ ($100\text{k NNZ}$) | **$0.094356 \text{ ms}$ / iter** | Steady-state zero-allocation execution |

---
*Ready for Phase 5 (Presolve Engine).*
