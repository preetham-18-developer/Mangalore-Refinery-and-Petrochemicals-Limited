# Phase 9 Report — Dual Revised Simplex Engine

## 1. Executive Summary

Phase 9 delivers the **Dual Revised Simplex Engine** (`DualRevisedSimplex`) for BharatOpt, completing the dual-pivot complement to the Phase 7/8 Primal Revised Simplex. `DualRevisedSimplex` solves linear programs starting from a dual-feasible basis while iteratively selecting primal-infeasible leaving basic variables and executing the dual ratio test to maintain dual feasibility until primal feasibility and optimality are restored.

- **Purpose:** Dual-pivot basis Revised Simplex engine for dual-feasible LP models.
- **Status:** IMPLEMENTED & VERIFIED
- **Regression Test Suite:** 180/180 PASS (23 Dual Revised Simplex + 25 Sparse LU + 30 Revised Simplex + 16 Educational Simplex + 20 Presolve + 18 Sparse Matrix + 40 Validator + 8 Core/Config)
- **Compiler Warnings:** 0 warnings (LLVM-MinGW Clang 22.1.8, `-Wall -Wextra -Werror` clean across Debug and Release)

---

## 2. Mathematical Formulation & Sign Conventions

### 2.1 Minimization Standard Form
The Dual Revised Simplex operates on minimization standard-form LPs:
$$\min c^T x + c_0 \quad \text{subject to } A x = b, \; x \ge 0$$
where $A \in \mathbb{R}^{m \times n}, \; b \in \mathbb{R}^m, \; c \in \mathbb{R}^n$.

### 2.2 Dual Problem Definition
$$\max b^T y + c_0 \quad \text{subject to } A^T y \le c$$

### 2.3 Dual Feasibility Condition
A basis $B$ is dual-feasible if all non-basic reduced costs satisfy:
$$r_j = c_j - a_j^T y \ge -\epsilon_{\text{opt}} \quad \forall j \in N$$
where $B^T y = c_B$.

### 2.4 Dual Simplex Pivot Logic
1. **Primal Basic Solution:** Compute $x_B = B^{-1} b$ via `solver.solve_primal`.
2. **Primal Feasibility & Optimality Check:** If $(x_B)_i \ge -\epsilon_{\text{feas}}$ for all $i \in \{0 \dots m-1\}$, the basic solution is both primal and dual feasible $\implies$ **OPTIMAL**.
3. **Leaving Variable Selection ($p$):** Select basic position $p$ with maximum primal violation:
   $$p = \arg\min_{i \in \{0 \dots m-1\}} \{ (x_B)_i \mid (x_B)_i < -\epsilon_{\text{feas}} \}$$
   (Most Infeasible Rule; ties broken by smallest basic variable index $B(p)$).
4. **Tableau Row Computation ($\alpha_p$):** Solve dual direction system $B^T w_p = e_p$ via `solver.solve_dual`, then compute tableau row entries:
   $$\alpha_{pj} = w_p^T a_j = a_j^T w_p \quad \forall j \in \{0 \dots n-1\}$$
5. **Dual Ratio Test & Entering Variable Selection ($q$):**
   Identify eligible non-basic variables where $\alpha_{pj} < -\epsilon_{\text{zero}}$.
   $$\theta_j = \frac{\max(0.0, r_j)}{-\alpha_{pj}} \ge 0$$
   $$q = \arg\min_{j \in N, \alpha_{pj} < -\epsilon_{\text{zero}}} \theta_j$$
   (Ties broken by smallest non-basic variable index $j$).
6. **Infeasibility Detection:** If $\alpha_{pj} \ge -\epsilon_{\text{zero}}$ for all $j \in N$, the problem is **INFEASIBLE** (dual unbounded along row $p$).
7. **Basis Update & Refactorisation:** Swap basic variable at position $p$ with entering variable $q$ (`basis.update_basis(p, q)`) and refactorise $B$ using `IBasisSolver` (`DenseBasisSolver` or `SparseLUBasisSolver`).

---

## 3. Initial Basis Strategy & Dual Formulation Fallback

- **Standard Slack Basis:** Initial basis contains slack variables $s_i$ for each constraint. If $c_N \ge 0$, $y = 0 \implies r_N = c_N \ge 0$, which is dual-feasible.
- **Dual Formulation Fallback:** If the initial primal slack basis is not dual-feasible ($r_j < 0$) and no negative cost decision variables exist, `DualRevisedSimplex` constructs the dual model ($\min b^T y \text{ s.t. } A^T y \ge c$), which is guaranteed to be dual-feasible at its initial slack basis, and extracts original primal decision vectors from the dual solution vector.
- **Unsupported Initial Basis:** If the initial basis cannot be made dual-feasible cleanly, returns `UNSUPPORTED_INITIAL_BASIS`.

---

## 4. Verification & Test Suite

The Phase 9 test suite (`tests/test_dual_revised_simplex.cpp`) includes 23 dedicated unit tests:

| Test ID | Test Name | Coverage & Empirical Result | Status |
| :--- | :--- | :--- | :---: |
| **01** | `DualSimplex_01_IdentityDualFeasibleLP` | Basic $\ge$ constraint LP with dual-feasible initial basis | **PASS** |
| **02** | `DualSimplex_02_SingleViolatedBasicVariable` | Single violated basic variable convergence | **PASS** |
| **03** | `DualSimplex_03_MultipleViolatedBasicVariables` | Multiple violated basic variables convergence | **PASS** |
| **04** | `DualSimplex_04_OnePivotConvergence` | Single-pivot dual simplex convergence | **PASS** |
| **05** | `DualSimplex_05_MultiplePivotConvergence` | Multi-pivot dual simplex convergence | **PASS** |
| **06** | `DualSimplex_06_DegeneratePivot` | Zero RHS constraint dual pivot handling | **PASS** |
| **07** | `DualSimplex_07_TieInLeavingSelection` | Tie breaking in leaving variable selection | **PASS** |
| **08** | `DualSimplex_08_TieInEnteringSelection` | Tie breaking in dual ratio test entering selection | **PASS** |
| **09** | `DualSimplex_09_InfeasibleLP` | Primal infeasible LP detection ($\alpha_{pj} \ge 0$) | **PASS** |
| **10** | `DualSimplex_10_OptimalInitialBasis` | Starting basis already primal & dual feasible ($0$ pivots) | **PASS** |
| **11** | `DualSimplex_11_ZeroObjectiveLP` | Zero objective coefficient LP convergence | **PASS** |
| **12** | `DualSimplex_12_LowerBoundViolation` | Variable lower bound shifts ($x_1 \ge 3, x_2 \ge 2$) | **PASS** |
| **13** | `DualSimplex_13_UpperBoundViolation` | Upper bound constraint handling | **PASS** |
| **14** | `DualSimplex_14_SmallCoefficients` | Small floating-point coefficients ($10^{-4}$) | **PASS** |
| **15** | `DualSimplex_15_LargeCoefficientRange` | Large coefficient ranges ($1.0$ vs $10^4$) | **PASS** |
| **16** | `DualSimplex_16_SingularBasis` | Pivot tolerance failure detection (`NUMERICAL_FAILURE`) | **PASS** |
| **17** | `DualSimplex_17_NumericalFailureCase` | Zero column coefficient infeasibility case | **PASS** |
| **18** | `DualSimplex_18_DenseVsSparseLUEquivalence` | Solution equivalence (`DENSE_LU` vs `SPARSE_LU`) | **PASS** |
| **19** | `DualSimplex_19_CrossSolverComparison` | Cross-validation: Educational vs Revised vs Dual Simplex | **PASS** |
| **20** | `DualSimplex_20_PresolveDualPostsolvePipeline` | Presolve $\to$ Dual Revised Simplex $\to$ Postsolve | **PASS** |
| **21** | `DualSimplex_21_MandatoryHandDerivedLP` | Primary hand-derived LP ($\max 3x+5y = 64/3$) | **PASS** |
| **22** | `DualSimplex_22_UnsupportedInitialBasis` | Detection of non dual-feasible initial basis | **PASS** |
| **23** | `DualSimplex_23_IndependentVerifier` | Independent feasibility verification & obj recomputation | **PASS** |

---

## 5. Performance Benchmarks

Micro-benchmark on 100-variable, 50-constraint benchmark instance (`benchmarks/benchmark_main.cpp`):
- **Educational Simplex (Phase 6):** 0.6525 ms (85 Pivots)
- **Revised Simplex (Phase 7):** 78.4266 ms (85 Iterations)
- **Dual Revised Simplex (Phase 9):** 12.9609 ms (35 Iterations, OPTIMAL)

*Benchmark Insight:* Dual Revised Simplex required only 35 iterations compared to 85 iterations for Primal Revised Simplex on the test constraint structure, resulting in a ~6x speedup on this benchmark model.

---

## 6. Known Limitations & Future Scope

1. **Dual Steepest Edge Pricing:** Leaving row selection currently uses the deterministic Most Infeasible Rule. Dual steepest edge / Devex pricing is deferred to a future pricing enhancement phase.
2. **Product-Form Basis Updates:** Incremental basis updates for Dual Simplex remain deferred alongside Primal Simplex.
