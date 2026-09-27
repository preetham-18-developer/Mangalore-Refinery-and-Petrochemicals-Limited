# Phase 19 Plan — MILP LP Warm Starts & Incremental Node Re-Optimization

## 1. Overview
Phase 19 focuses on **LP warm-starting and incremental re-optimization** between parent and child Branch-and-Bound (B&B) nodes. In Phase 18, characterisation revealed that cumulative LP relaxation solve time accounts for 85.4%–92.1% of total B&B execution time because child node LPs are solved from scratch.

By reusing the parent node's optimal basis and dual-feasible state when solving child LP relaxations (which differ from the parent only by variable bound tightening), Phase 19 aims to reduce child LP solve iterations without altering the mathematical search decisions or pruning rules of the B&B engine.

---

## 2. Reusable Infrastructure & Architecture Findings

### Reusable Components:
- **`DualRevisedSimplex`** ([dual_revised_simplex.hpp](file:///C:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/include/bharatopt/dual_revised_simplex.hpp)): Dual simplex kernel naturally suited for warm-starting when variable bounds are tightened.
- **`Basis` & `IBasisSolver`** ([revised_simplex.hpp](file:///C:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/include/bharatopt/revised_simplex.hpp)): Basis structure, basic/nonbasic partitions, and factorization interface.
- **`BranchAndBoundEngine`** ([branch_and_bound.hpp](file:///C:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/include/bharatopt/branch_and_bound.hpp)): B&B search engine, node hierarchy, and telemetry.

### New Components Required:
1. **`LPWarmStartState`**: Struct capturing parent basis, variable statuses, primal/dual solutions, and structural metadata.
2. **`DualRevisedSimplex::solve_warm_start`**: Entry point accepting a pre-existing `Basis` and performing dual-simplex re-optimization.
3. **Compatibility Checker**: Structural validator checking dimensions, variable bounds, and basis invariants before warm-starting.
4. **Safe Cold-Start Fallback**: Mechanism that safely catches warm-start rejections/failures and falls back to Phase 17 cold-start solving.
5. **Cold vs Warm A/B Execution Modes**: `WarmStartMode::COLD_START` vs `WarmStartMode::WARM_START` configuration.
6. **Telemetry Additions**: Tracking warm-start attempts, acceptances, rejections, failures, and cold fallbacks.

---

## 3. Mathematical Warm-Start Contract

When a branching variable $x_{j^*}$ is bounded in a child node ($x_{j^*} \le \lfloor v \rfloor$ or $x_{j^*} \ge \lceil v \rceil$):
1. **Matrix Invariance:** Matrix $A$ and objective $c$ remain identical to parent.
2. **Dual Feasibility:** The parent optimal basis $B$ remains dual-feasible ($r_N = c_N - A_N^T B^{-T} c_B \ge 0$).
3. **Primal Infeasibility:** The parent solution $x_B = B^{-1} b$ may violate the new bound constraint.
4. **Re-Optimization:** `DualRevisedSimplex` starts from basis $B$ and performs dual pivots to restore primal feasibility in 1 to 5 iterations (compared to 20–50+ iterations for cold-start Phase 1/2).

---

## 4. Fallback & Verification Safety Rules

- If parent basis fails structural compatibility checks ($m \neq m_{\text{parent}}$ or invalid basis indices): **REJECT** warm-start $\rightarrow$ **COLD FALLBACK**.
- If dual simplex warm-start returns numerical failure or unboundedness: **FAIL** warm-start $\rightarrow$ **COLD FALLBACK**.
- Numerical failures will **NEVER** be interpreted as node infeasibility.
- All warm-started child LP solutions must pass independent solution feasibility verification.

---

## 5. Testing & Regression Strategy
- **Equivalence Verification:** Run `COLD_START` and `WARM_START` modes across all Phase 17/18 correctness models and verify exact solution status and objective matching ($\le 10^{-4}$).
- **Dedicated Phase 19 Unit Tests:** 10 dedicated tests covering basic warm start, bound tightening, fractional parent, integer root, infeasible child, bound pruning, cold/warm equivalence, model immutability, warm-start rejection, and failure fallback.
- **Full Suite Regression:** Ensure all 318 previous unit tests continue to pass with 0 errors and 0 compiler warnings in Debug & Release builds.
