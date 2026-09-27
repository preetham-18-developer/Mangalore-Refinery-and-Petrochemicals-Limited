# Phase 18 Plan — MILP Benchmark Matrix & B&B Workload Characterisation

## 1. Overview
Phase 18 is a dedicated **measurement, characterisation, and benchmarking** phase for the CPU Branch-and-Bound (B&B) engine introduced in Phase 17. The mathematical behavior and search logic of the Phase 17 solver remain strictly unchanged.

The objective is to establish a deterministic, reproducible MILP benchmark matrix across 10 distinct workload dimensions (M1 through M10), collect comprehensive performance and tree telemetry, perform independent verification and brute-force cross-checking, and identify performance bottlenecks to guide future optimization phases.

---

## 2. Reusable Infrastructure & Architecture Findings

### Reusable Components:
- **`LPModel` & `ModelValidator`** ([lp_model.hpp](file:///C:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/include/bharatopt/lp_model.hpp)): Model representations, variable classification, bounds, and consistency checks.
- **`BranchAndBoundEngine`** ([branch_and_bound.hpp](file:///C:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/include/bharatopt/branch_and_bound.hpp)): Phase 17 B&B solver engine and telemetry structure.
- **`MilpFoundation`** ([milp_foundation.hpp](file:///C:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/include/bharatopt/milp_foundation.hpp)): Continuous LP relaxation extraction and integer feasibility checker.
- **`BenchmarkSuite` / `BenchmarkReporter`** ([benchmark_framework.hpp](file:///C:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/include/bharatopt/benchmark_framework.hpp)): Seed-based random number generation, CSV/JSON export routines, and summary reporting.

### New Components Required:
1. **`MilpBenchmarkInstanceConfig`**: Configuration struct for seed-based MILP instance generation.
2. **`MilpBenchmarkResultRecord`**: Data structure holding model metrics, B&B tree telemetry, LP breakdown, verification, and brute-force comparison.
3. **`MilpBenchmarkGenerator`**: Seed-based generator creating deterministic MILPs with known integer feasibility.
4. **`BruteForceMilpSolver`**: Independent brute-force reference solver for small bounded integer/binary MILPs.
5. **`MilpBenchmarkRunner`**: Runner executing individual MILP benchmarks, measuring node and LP timings, and validating solutions.
6. **`MilpBenchmarkSuite`**: Suite covering Categories M1–M10 (Dimension Scaling, Integer Density, Binary vs Integer, Sparsity, Fractionality, Tree Growth, Objective Sense, Numerical Scale, Hand-Derived Cases, and Brute-Force Cross-Check).
7. **CSV/JSON Exporters**: Exporters for `phase-18-milp-benchmark.csv` and `phase-18-milp-benchmark.json`.

---

## 3. MILP Benchmark Categories (M1–M10)

| Category | Description | Key Variable Dimension |
| :--- | :--- | :--- |
| **M1: Dimension Scaling** | 10v/5c up to 500v/250c | Total variables & constraints |
| **M2: Integer Density** | 0%, 10%, 25%, 50%, 75%, 100% | Integer variable fraction |
| **M3: Binary vs Integer** | Binary-heavy vs Integer-heavy vs Mixed | Binary to Integer ratio |
| **M4: Sparsity** | 0.1%, 0.5%, 1%, 5%, 10% | Constraint matrix density |
| **M5: Fractionality** | Integer-root, Mildly-fractional, Substantially-fractional | Distance of root LP solution from integer |
| **M6: Tree Growth** | Shallow, Moderate, Deep trees | Measured tree depth & node count |
| **M7: Objective Sense** | MAXIMIZE vs MINIMIZE | Objective direction & lower/upper bound pruning |
| **M8: Numerical Scale** | Coefficients $10^{-4}$ to $10^{4}$ | Dynamic range & numerical stability |
| **M9: Hand-Derived** | Hand Cases 1–6 from Phase 17 | Known exact hand solutions |
| **M10: Brute-Force Cross-Check** | Small bounded MILPs | Exhaustive grid search cross-check |

---

## 4. Measurement & Telemetry Protocol

For every MILP benchmark, the following metrics will be recorded:

1. **Model Metrics:** Rows, Columns, NNZ, Density, Continuous Count, Integer Count, Binary Count, Objective Sense, Coeff Min/Max, Dynamic Range.
2. **B&B Tree Telemetry:** Status, Total Solve Time, Root LP Time, Nodes Created, Nodes Processed, Max Depth, Nodes Pruned by Infeasibility, Nodes Pruned by Bound, Integer-Feasible Nodes, Incumbent Updates, Final Objective, Best Open Node Bound, Node Limit Status.
3. **LP Relaxation Breakdown:** LP Solve Count, Cumulative LP Time, Avg LP Time, Min/Max LP Time.
4. **Independent Verification:** Constraint Residual, Bound Violation, Integrality Violation, Recomputed Objective, Verification Status.
5. **Brute-Force Reference (M10):** Brute-force evaluated count, Brute-force optimum objective, Match status.

---

## 5. Brute-Force Cross-Check Methodology
For small bounded integer/binary problems ($\prod_{j \in I} (u_j - l_j + 1) \le 100,000$), an independent brute-force solver evaluates all integer grid vectors:
1. Enumerates all integer assignments within variable lower/upper bounds.
2. Evaluates continuous variables (if applicable) or verifies inequality constraints $A x \le b / A x \ge b$.
3. Identifies the exact global optimum objective.
4. Compares with the objective returned by the `BranchAndBoundEngine`.

---

## 6. Verification & Regression Plan
- Build Debug & Release versions with 0 compiler errors and 0 warnings.
- Run baseline regression test suite (306/306 passing).
- Run dedicated Phase 18 unit tests (`tests/test_milp_benchmark.cpp`).
- Export benchmark artifacts to `benchmarks/results/phase_18_milp_benchmark.csv` and `benchmarks/results/phase_18_milp_benchmark.json`.

---

## 7. Risks & Mitigation
- **Risk:** Combinatorial explosion causing benchmark timeouts on large synthetic MILPs.
  - **Mitigation:** Enforce strict node limits (`max_nodes = 500` or `1000`) and record `LIMIT_REACHED` status accurately without marking as `OPTIMAL`.
- **Risk:** Numerical instability in LP relaxation causing node failures.
  - **Mitigation:** Standardize feasibility/optimality tolerances ($10^{-7}$) and record `NUMERICAL_FAILURE` explicitly.
