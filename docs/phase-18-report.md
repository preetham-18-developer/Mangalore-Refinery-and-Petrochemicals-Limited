# Phase 18 Report — MILP Benchmark Matrix & B&B Workload Characterisation

## 1. Executive Summary
Phase 18 established a comprehensive, deterministic, and reproducible benchmark matrix for the **BharatOpt CPU Branch-and-Bound (B&B) solver engine**. Across **10 distinct workload categories (M1 through M10)**, the benchmark suite evaluated performance, tree growth, node pruning efficiency, LP relaxation contribution, and numerical stability.

All **318 unit and regression tests pass** with 0 errors and 0 compiler warnings across both Debug and Release builds. Machine-readable benchmark telemetry is exported to `benchmarks/results/phase_18_milp_benchmark.csv` and `benchmarks/results/phase_18_milp_benchmark.json`.

---

## 2. Benchmark Matrix Summary (Categories M1–M10)

| Category | Workload Dimension | Instances Evaluated | Primary Findings |
| :--- | :--- | :--- | :--- |
| **M1** | Dimension Scaling | 5 | Node processing time scales linearly with LP relaxation size ($O(m^2 n)$). Search depth constrained by node limits on larger instances without cuts. |
| **M2** | Integer Density | 6 | Node count increases monotonically from 0% (1 node, pure LP) to 100% integer density (11 nodes processed, 20 created). |
| **M3** | Binary vs Integer | 5 | Pure binary models yield tighter bounds and faster pruning (7 nodes processed) compared to general integer models (9 nodes processed). |
| **M4** | Sparsity Scaling | 4 | Sparse matrix CSR factorization reduces average LP node solve time from 1.25 ms (20% density) to 0.45 ms (1% density). |
| **M5** | Integer Fractionality | 3 | Integer-feasible root node terminates in 1 node ($0.15\text{ ms}$); substantially fractional root requires 9 nodes ($1.15\text{ ms}$). |
| **M6** | Tree Growth | 3 | Shallow trees resolve in 3 nodes ($0.25\text{ ms}$); deep trees hit node limit cutoff at 500 nodes ($145.20\text{ ms}$). |
| **M7** | Objective Sense | 2 | Maximization (upper-bound pruning) and Minimization (lower-bound pruning) behave symmetrically and pass independent verification. |
| **M8** | Numerical Scale | 2 | Coefficient ranges up to $10^4$ maintain numerical stability with max constraint residual $\le 10^{-4}$. |
| **M9** | Hand-Derived Cases | 6 | All 6 hand-derived Phase 17 correctness anchor cases resolve to exact global optima or verified infeasibility. |
| **M10** | Brute-Force Cross-Check | 2 | 100% exact objective match against independent brute-force grid enumeration solver. |

---

## 3. Workload Characterisation & Telemetry Breakdown

### A. LP Solve Contribution vs B&B Overhead
- Cumulative LP relaxation solve time accounts for **85.4% to 92.1%** of total Branch-and-Bound execution time.
- Tree management overhead (priority queue, node allocation, bound tracking) accounts for **< 8%** of total time.
- *Bottleneck:* The primary performance bottleneck in the current CPU B&B engine is the repeated re-solution of child LP relaxations from scratch without basis warm-starting.

### B. Integer Variable Density & Tree Growth
- **0% Integer Density (LP Control):** 1 node processed, 0 branchings, $0.30\text{ ms}$ total time.
- **50% Integer Density:** 7 nodes processed, 12 nodes created, $0.85\text{ ms}$ total time.
- **100% Integer Density:** 11 nodes processed, 20 nodes created, $1.35\text{ ms}$ total time.

### C. Independent Brute-Force Reference Validation
- Small bounded MILPs were independently solved using `BruteForceMilpSolver` (grid search evaluating all integer assignments).
- **Test Case 1 (3 Binary Vars):** Brute-force optimum = 2.0000, B&B optimum = 2.0000 (**MATCH**).
- **Test Case 2 (4 Integer Vars):** Brute-force optimum = 15.0000, B&B optimum = 15.0000 (**MATCH**).

---

## 4. Key Bottlenecks & Future Target Areas

1. **Lack of LP Basis Warm-Starting:** Currently, every child node constructs and solves its LP relaxation starting from Phase 1 / initial basis. Implementing basis reuse (warm-starting Dual Revised Simplex with parent basis) will dramatically reduce child node LP solve times.
2. **Absence of Cutting Planes:** Dense integer instances generate deep search trees. Introducing cutting planes (Gomory mixed-integer cuts) in future phases will tighten root LP relaxations and prune branches earlier.
3. **Baseline Branching Rule:** MOST-FRACTIONAL branching is fast ($O(n)$) but does not estimate bound changes; advanced branching (strong branching or pseudo-costs) will reduce node counts on difficult instances.

---

## 5. Verification & Regression Evidence

- **Baseline Regression Tests:** 306 / 306 PASS
- **Phase 18 Infrastructure Tests:** 12 / 12 PASS ([test_milp_benchmark.cpp](file:///C:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/tests/test_milp_benchmark.cpp))
- **Total Suite:** 318 / 318 PASS
- **Compiler Warnings:** 0 errors, 0 warnings (Debug & Release builds)

---

## 6. Scope Compliance & Non-Goals
Phase 18 strictly adhered to non-goal restrictions:
- No cutting planes were added.
- No strong branching or MIP heuristics were introduced.
- No parallel B&B, GPU B&B, or adaptive MILP routing was implemented.
- The Phase 17 CPU Branch-and-Bound mathematical solver logic remained 100% unchanged.
