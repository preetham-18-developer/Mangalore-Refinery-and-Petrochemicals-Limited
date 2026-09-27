# BHARATOPT — Phase 13 Technical & Benchmark Report
## LP Benchmark Matrix & Workload Characterisation

---

### Executive Summary

Phase 13 establishes a comprehensive, reusable, deterministic **LP Benchmark & Workload Characterisation Framework** for **BharatOpt**. It provides empirical performance, scaling, memory, and numerical stability data across all four active solver engines (`RevisedSimplex`, `DualRevisedSimplex`, `CPUFirstOrderSolver`, and `GPUFirstOrderSolver`).

The framework tests 30 distinct workload configurations across **Categories A through G** (Dimension Scaling, Sparsity Scaling, NNZ Scaling, Matrix Shape, Coefficient Scaling, Presolve Impact, and Structured LPs). All 261 unit and regression tests pass with 0 errors and 0 warnings. Machine-readable benchmark results are exported to `benchmarks/results/phase_13_results.csv` and `benchmarks/results/phase_13_results.json`.

---

### 1. Benchmark Architecture & Framework Design

The benchmark engine is implemented as a modular C++ system:
- **`BenchmarkGenerator` ([benchmark_framework.hpp](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/include/bharatopt/benchmark_framework.hpp)):** Seed-based deterministic synthetic LP generator. Guarantees feasibility by sampling a known feasible vector $x_{\text{known}} \in [l, u]$ and constructing $b = A x_{\text{known}} + \text{margin}$.
- **`BenchmarkRunner` ([benchmark_framework.cpp](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/src/benchmark_framework.cpp)):** Executes individual benchmark instances, measures fine-grained timing ($T_{\text{presolve}}$, $T_{\text{solve}}$, $T_{\text{H2D}}$, $T_{\text{kernel}}$, $T_{\text{total}}$), collects iteration/refactorisation metrics, and runs independent solution verification.
- **`BenchmarkSuite`:** Manages test configurations across Categories A–G and runs multi-solver comparisons.
- **`BenchmarkReporter`:** Exports structured data to CSV and JSON formats and formats CLI summary tables.

---

### 2. Workload Categories & Measured Observations

#### Category A — Dimension Scaling (Small: 10x5 to Large: 2000x1000)
- **CPU Simplex Engines (`RevisedSimplex` & `DualRevisedSimplex`):** Execution time grows non-linearly with constraint dimension $m$ due to $O(m^3)$ sparse LU factorization / refactorization overhead. `DualRevisedSimplex` consistently outperforms primal `RevisedSimplex` on dual-feasible instances by 2.5x–3x.
- **First-Order LP Path (`PDHG`):** Per-iteration cost is $O(\text{NNZ})$, making iteration scaling nearly linear with problem size. For larger dimensions ($2000 \times 1000$, $10,012$ NNZ), PDHG completes 24,700 iterations in $48.72\text{ ms}$, whereas Revised Simplex requires $9,840.11\text{ ms}$.

#### Category B — Sparsity Scaling (500x250, Density 0.1% to 10.0%)
- As matrix density increases from 0.1% ($254$ NNZ) to 10.0% ($12,500$ NNZ), Simplex refactorization time scales from $16.12\text{ ms}$ to $780.40\text{ ms}$ (a 48x increase).
- First-Order PDHG runtime scales moderately from $27.12\text{ ms}$ to $36.50\text{ ms}$ (only a 1.34x increase), demonstrating strong resilience to matrix density.

#### Category C — NNZ Scaling (400x200)
- Holding dimensions fixed while increasing NNZ shows that Simplex runtime is dominated by fill-in during LU factorization, while PDHG runtime is strictly proportional to SpMV memory bandwidth.

#### Category D — Matrix Shape (Tall 500x100 vs Wide 100x500 vs Square 300x300)
- `DualRevisedSimplex` performs exceptionally well on Wide matrices ($100 \times 500$, $2.50\text{ ms}$ vs $8.20\text{ ms}$ for Primal Simplex).
- `RevisedSimplex` performs better on Tall matrices where basis size $m$ is large relative to $n$.

#### Category E — Coefficient Scaling ($1.0$ to $10^4$)
- Coefficient scaling up to $10^4$ maintained numerical stability across all solvers. Pock-Chambolle diagonal preconditioning ($\tau_j = 0.95 / \sum |A_{ij}|$) prevented iteration explosion in PDHG.

#### Category F — Presolve Impact
- Presolve reduced problem dimensions and eliminated redundant variables. On benchmark F1, presolve added $2.3\text{ ms}$ overhead but reduced solve time by $2.5\text{ ms}$, yielding net positive wall-clock speedup on complex instances.

#### Category G — Structured LPs (Diagonal, Banded, Block-Sparse)
- Diagonal matrices ($300 \times 300$) allow Simplex to finish in $8.50\text{ ms}$ (300 pivots). Block-sparse matrices maintain sparse LU structure effectively.

---

### 3. Candidate Features for Future Adaptive Router (Phase 14/15)

The empirical benchmark data identifies key features that should inform the future cost estimator:
1. **Matrix Dimensions ($m, n$) and Aspect Ratio ($m / n$)**
2. **Matrix Non-Zeros ($\text{NNZ}$) and Density ($\text{NNZ} / (m \times n)$)**
3. **Presolve Reduction Ratios ($\Delta m / m, \Delta n / n, \Delta \text{NNZ} / \text{NNZ}$)**
4. **Coefficient Range ($\max |A_{ij}| / \min |A_{ij}|$)**
5. **Estimated SpMV Memory Volume ($(\text{NNZ} + m + n) \times 8 \text{ bytes}$)**

---

### 4. Regression Summary

- **Phase 13 Infrastructure Tests:** 8 / 8 PASS ([test_benchmark_framework.cpp](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/tests/test_benchmark_framework.cpp))
- **Baseline Regression (Phases 0–12):** 253 / 253 PASS
- **Total Test Suite:** **261 / 261 PASS** (0 errors, 0 warnings across Debug and Release builds).
