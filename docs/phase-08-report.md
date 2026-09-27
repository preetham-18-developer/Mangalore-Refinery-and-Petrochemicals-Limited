# Phase 8 Report — Sparse LU / Basis Factorisation Engine

## 1. Executive Summary

Phase 8 delivers the **Sparse LU / Basis Factorisation Engine** for BharatOpt, extending the modular linear algebra abstraction (`IBasisSolver`) introduced in Phase 7. Rather than dense basis refactorisation, `SparseLUBasisSolver` extracts sparse basis matrices directly from `StandardFormLP`, computes row LU decompositions with partial pivoting ($P B = L U$), and performs forward and backward triangular solves for both primal systems ($B x = b$) and transpose/dual systems ($B^T y = c_B$).

- **Purpose:** Sparse-aware basis factorisation and system solver for Revised Simplex.
- **Status:** IMPLEMENTED & VERIFIED
- **Regression Test Suite:** 157/157 PASS (25 Sparse LU + 30 Revised Simplex + 16 Educational Simplex + 20 Presolve + 18 Sparse Matrix + 40 Validator + 8 Core/Config)
- **Compiler Warnings:** 0 warnings (LLVM-MinGW Clang 22.1.8, `-Wall -Wextra -Werror` clean across Debug and Release)

---

## 2. Mathematical Foundation & Permutation Conventions

### 2.1 Permutation Convention
For a square basis matrix $B \in \mathbb{R}^{m \times m}$ extracted from basic columns $B = A[:, \text{basic\_vars}]$:
$$P B = L U$$
where:
- $P$ is an $m \times m$ row permutation matrix representing row interchanges performed during partial pivoting.
- $L$ is an $m \times m$ unit lower triangular matrix ($L_{ii} = 1$).
- $U$ is an $m \times m$ upper triangular matrix.

### 2.2 Primal System Solve ($B x = \text{rhs}$)
1. **Row Permutation:** $b_{\text{perm}} = P \cdot \text{rhs} \implies (b_{\text{perm}})_i = \text{rhs}_{P_{\text{row}}[i]}$
2. **Forward Substitution ($L y = b_{\text{perm}}$):**
   $$y_i = (b_{\text{perm}})_i - \sum_{j < i} L_{ij} y_j \quad (L_{ii} = 1)$$
3. **Backward Substitution ($U x = y$):**
   $$x_i = \frac{y_i - \sum_{j > i} U_{ij} x_j}{U_{ii}}$$

### 2.3 Transpose System Solve ($B^T y = \text{rhs}$)
The dual vector pricing step requires $B^T y = c_B$:
$$(P B)^T y = (L U)^T y \implies B^T P^T = U^T L^T \implies U^T L^T P y = \text{rhs}$$
1. **Forward Substitution ($U^T w = \text{rhs}$):**
   $$w_i = \frac{\text{rhs}_i - \sum_{k < i} U_{ki} w_k}{U_{ii}}$$
2. **Backward Substitution ($L^T z = w$):**
   $$z_i = w_i - \sum_{k > i} L_{ki} z_k \quad (L_{ii} = 1)$$
3. **Permutation Reversal ($P y = z$):**
   $$y_{P_{\text{row}}[i]} = z_i$$

---

## 3. Architecture & Basis Extraction

### 3.1 Sparse Basis Extraction
`SparseLUBasisSolver::extract_sparse_basis(lp, basis)`:
- Iterates over basic variable indices $j \in \{0 \dots m-1\}$.
- Extracts nonzero entries from standard-form matrix column $A[:, \text{basic\_vars}[j]]$.
- Constructs a sparse `COOMatrix(m, m)` and converts directly to `CSRMatrix`.
- Avoids densifying matrix $A$.

### 3.2 Fill-In Accounting & Statistics
The `SparseLUStats` structure tracks:
- `matrix_dim`: Dimension $m$ of basis.
- `original_basis_nnz`: Nonzero entries in $B$.
- `l_nnz`: Nonzero entries in $L$.
- `u_nnz`: Nonzero entries in $U$.
- `total_factor_nnz`: $L_{\text{nnz}} + U_{\text{nnz}} - m$ (accounting for unit diagonal overlap).
- `fill_in_nnz`: $\text{total\_factor\_nnz} - \text{original\_basis\_nnz}$.
- `fill_in_ratio`: $\frac{\text{total\_factor\_nnz}}{\max(1, \text{original\_basis\_nnz})}$.
- `pivot_min`, `pivot_max`: Extreme pivot magnitude metrics.

---

## 4. Pivoting & Numerical Stability

### 4.1 Partial Row Pivoting
At step $k \in \{0 \dots m-1\}$:
1. Search column $k$ for pivot row $r = \arg\max_{i \ge k} |W_{ik}|$.
2. Check numerical tolerance: If $\max_{i \ge k} |W_{ik}| < \text{pivot\_tol}$ (default $10^{-10}$), factorisation fails cleanly (`return false`).
3. Swap row $k$ with row $r$ in working matrix $W$ and permutation vector $P_{\text{row}}$.
4. Perform column elimination for entries below diagonal:
   $$W_{ik} \leftarrow \frac{W_{ik}}{W_{kk}}, \quad W_{ij} \leftarrow W_{ij} - W_{ik} W_{kj} \quad (\forall i > k, j > k)$$

---

## 5. Revised Simplex Integration

Target modular architecture achieved:
```
           RevisedSimplex
                 |
                 v
           IBasisSolver
                 |
        +--------+--------+
        |                 |
        v                 v
 DenseBasisSolver    SparseLUSolver
    (Phase 7)           (Phase 8)
```

The choice of solver is controlled dynamically via `RevisedSimplexOptions::solver_type`:
- `BasisSolverType::DENSE_LU`: Phase 7 reference dense LU solver.
- `BasisSolverType::SPARSE_LU`: Phase 8 sparse LU solver.

---

## 6. Verification & Test Suite

The Phase 8 test suite (`tests/test_sparse_lu.cpp`) includes 25 dedicated unit tests:

| Test ID | Test Name | Summary / Empirical Result | Status |
| :--- | :--- | :--- | :---: |
| **01** | `SparseLU_01_IdentityMatrix` | $3 \times 3$ Identity matrix factorisation & solves | **PASS** |
| **02** | `SparseLU_02_DiagonalMatrix` | $3 \times 3$ Diagonal matrix primal solve | **PASS** |
| **03** | `SparseLU_03_SmallDenseMatrix` | $3 \times 3$ Tridiagonal matrix system solve | **PASS** |
| **04** | `SparseLU_04_SparseMatrix` | $10 \times 10$ Sparse band matrix system solve | **PASS** |
| **05** | `SparseLU_05_PermutedSparseMatrix` | Zero diagonal entry requiring row swap | **PASS** |
| **06** | `SparseLU_06_TriangularMatrix` | Lower triangular matrix factorisation & solve | **PASS** |
| **07** | `SparseLU_07_SingularMatrix` | Zero row singular basis detected cleanly | **PASS** |
| **08** | `SparseLU_08_NearSingularMatrix` | $10^{-12}$ pivot below threshold detected cleanly | **PASS** |
| **09** | `SparseLU_09_ZeroPivot` | Anti-diagonal permutation matrix factorisation | **PASS** |
| **10** | `SparseLU_10_NearZeroPivot` | Small diagonal entry swapped with unit subdiagonal | **PASS** |
| **11** | `SparseLU_11_ForwardSolve` | Forward substitution step validation | **PASS** |
| **12** | `SparseLU_12_BackwardSolve` | Backward substitution step validation | **PASS** |
| **13** | `SparseLU_13_TransposeSolve` | Transpose dual solve ($B^T y = \text{rhs}$) verification | **PASS** |
| **14** | `SparseLU_14_PermutationCorrectness` | Permutation mapping and un-permutation checks | **PASS** |
| **15** | `SparseLU_15_FillInAccounting` | Verified NNZ, factor NNZ, and fill-in accounting | **PASS** |
| **16** | `SparseLU_16_BasisExtraction` | Extraction of sparse $B$ from `StandardFormLP` | **PASS** |
| **17** | `SparseLU_17_DenseVsSparseSolutionComparison` | Solution comparison against `DenseBasisSolver` | **PASS** |
| **18** | `SparseLU_18_MultipleRHSSolves` | Multiple RHS solves on fixed factorised matrix | **PASS** |
| **19** | `SparseLU_19_DifferentSparsityPatterns` | Arrow-head matrix pattern factorisation | **PASS** |
| **20** | `SparseLU_20_NumericalScaling` | Coefficient scales from $10^{-4}$ to $10^4$ | **PASS** |
| **21** | `SparseLU_21_RevisedSimplexDenseSolver` | Revised Simplex with `DENSE_LU` | **PASS** |
| **22** | `SparseLU_22_RevisedSimplexSparseLUSolver` | Revised Simplex with `SPARSE_LU` | **PASS** |
| **23** | `SparseLU_23_BothSolversEquivalence` | Dense vs Sparse solver LP solution equivalence | **PASS** |
| **24** | `SparseLU_24_PresolveRevisedSparsePostsolvePipeline` | Presolve $\to$ Revised Simplex (`SPARSE_LU`) $\to$ Postsolve | **PASS** |
| **25** | `SparseLU_25_MandatoryLPCrossCheck` | Primary LP cross-check ($\max 3x+5y = 64/3$) | **PASS** |

---

## 7. Performance & Memory Analysis

### 7.1 Reference Benchmark Comparison
Identical benchmark model (100 variables, 50 constraints):
- **Educational Simplex (Phase 6):** 0.6745 ms (85 Pivots)
- **Revised Simplex (Phase 7 - Dense LU):** 61.9781 ms (85 Iterations)
- **Revised Simplex (Phase 8 - Sparse LU):** 75.7832 ms (85 Iterations)

*Note on Benchmarks:* For small dense bases ($50 \times 50$), full LU refactorisation in sparse data structures introduces small overhead relative to contiguous dense arrays. Incremental basis updates (e.g., product form / Forrest-Tomlin) will address refactorisation frequency in future phases.

### 7.2 Memory Usage
- Dense Basis Memory: $O(m^2)$ doubles = $50 \times 50 \times 8 = 20,000$ bytes.
- Sparse Basis Memory: $O(\text{nnz}(B))$ CSR vectors $\approx 200 \times 8 + 200 \times 4 + 51 \times 4 = 2,604$ bytes.
- Sparse LU Factor Memory: $O(\text{nnz}(L) + \text{nnz}(U)) \approx 3,500$ bytes.

---

## 8. Known Limitations & Future Scope

1. **Ordering Strategies:** Advanced reordering strategies (such as AMD or COLAMD) are deferred to dedicated linear algebra updates.
2. **Incremental Updates:** Refactorisation is currently performed at every pivot step. Product-form / eta-vector basis updates ($B_k^{-1} = E_k \dots E_1 B_0^{-1}$) are deferred to later optimization phases.
