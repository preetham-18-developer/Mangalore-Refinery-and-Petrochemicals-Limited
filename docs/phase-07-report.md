# Phase 7 Report — Revised Simplex Engine

## 1. Executive Summary

Phase 7 delivers the **Revised Simplex Engine** for BharatOpt, transitioning from the dense-tableau reference solver of Phase 6 to a production-oriented, basis-oriented Revised Simplex implementation. Rather than maintaining and updating a full dense simplex tableau at every iteration, `RevisedSimplex` maintains a compact `Basis` representation, solving linear primal systems $B x_B = b$, dual systems $B^T y = c_B$, and direction systems $B d_B = a_q$ via a modular `IBasisSolver` interface.

- **Purpose:** Production-oriented basis Revised Simplex engine.
- **Status:** IMPLEMENTED & VERIFIED
- **Regression Test Suite:** 132/132 PASS (30 Revised Simplex + 16 Educational Simplex + 20 Presolve + 18 Sparse Matrix + 40 Validator + 8 Core/Config)
- **Compiler Warnings:** 0 warnings (LLVM-MinGW Clang 22.1.8, `-Wall -Wextra -Werror` clean across Debug and Release)

---

## 2. Mathematical Formulation & Sign Conventions

### 2.1 Minimization Standard Form
The Revised Simplex engine operates on standard-form minimization LPs:
$$\min c^T x + c_0 \quad \text{subject to } A x = b, \; x \ge 0$$
where $A \in \mathbb{R}^{m \times n}$, $b \ge 0$. (Maximization LPs are converted by setting $c \leftarrow -\tilde{c}$).

### 2.2 Mathematical Conventions & Derivations
1. **Primal Basic Solution:**
   $$B x_B = b \implies x_B = B^{-1} b$$
   Non-basic variables: $x_N = 0$.
2. **Dual Vector Calculation (Pricing):**
   $$B^T y = c_B \implies y^T = c_B^T B^{-1}$$
3. **Reduced Cost Calculation:**
   $$r_j = c_j - y^T a_j = c_j - a_j^T y \quad \forall j \in N$$
   where $a_j$ is column $j$ of $A$.
4. **Optimality Criterion:**
   $$r_j \ge -10^{-7} \quad \forall j \in N$$
5. **Entering Variable Selection ($q$):**
   - **Bland's Anti-Cycling Rule:** $q = \min \{ j \in N \mid r_j < -10^{-7} \}$.
   - **Dantzig's Most Negative Rule:** $q = \arg\min_{j \in N} \{ r_j \mid r_j < -10^{-7} \}$.
6. **Step Direction Calculation ($d_B$):**
   $$B d_B = a_q \implies d_B = B^{-1} a_q$$
7. **Minimum Ratio Test & Leaving Variable Selection ($p$):**
   $$\theta_i = \frac{(x_B)_i}{(d_B)_i} \quad \text{for } (d_B)_i > 10^{-10}$$
   $$\lambda^* = \min_{i \in \{0 \dots m-1\}, (d_B)_i > 10^{-10}} \theta_i$$
   Leaving row index: $p = \arg\min_i \theta_i$. (Ties broken by smallest basic variable index $B(p)$).
   - **Unboundedness:** If $(d_B)_i \le 10^{-10}$ for all $i$, status = `UNBOUNDED`.

---

## 3. Basis Abstraction & Representation

The `Basis` class ([revised_simplex.hpp](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/include/bharatopt/revised_simplex.hpp)) maintains basic/non-basic state:
- `basic_vars`: Vector of $m$ basic variable indices.
- `nonbasic_vars`: Vector of $n - m$ non-basic variable indices.
- `var_to_basic_pos`: Array mapping $j \to$ position in `basic_vars` (-1 if non-basic).
- `var_to_nonbasic_pos`: Array mapping $j \to$ position in `nonbasic_vars` (-1 if basic).
- `var_status`: Vector tracking status (`BASIC`, `NONBASIC_LOWER`, `NONBASIC_UPPER`, `FREE`).

### Invariant Checks (`check_invariants()`)
1. $\text{basic\_vars.size()} == m$
2. $\text{nonbasic\_vars.size()} == n - m$
3. $\text{basic\_vars} \cap \text{nonbasic\_vars} = \emptyset$
4. Inverse mapping vectors strictly match array positions.

---

## 4. Modular Basis Solver Interface

To decouple Revised Simplex from specific basis factorizations:
- **`IBasisSolver`**: Abstract interface declaring `factorize`, `solve_primal` ($B x = b$), and `solve_dual` ($B^T y = c_B$).
- **`DenseBasisSolver`**: Phase 7 reference implementation using Gaussian Elimination with partial row pivoting ($P B = L U$).
- **Phase 8 Extension:** Phase 8 will introduce `SparseLUBasisSolver` to replace `DenseBasisSolver` seamlessly without modifying the core `RevisedSimplex` iteration loop.

---

## 5. Verification & Test Suite

The Phase 7 test suite (`tests/test_revised_simplex.cpp`) includes 30 dedicated unit tests:

| Test ID | Test Name | Coverage & Empirical Result | Status |
| :--- | :--- | :--- | :--- |
| **01** | `RevisedSimplex_01_Basic2VariableLP` | Primary hand-derived LP ($\max 3x+5y$) optimum at $(8/3, 8/3)$, $z = 64/3$ | **PASS** |
| **02** | `RevisedSimplex_02_MultiplePivotLP` | 3-variable, 3-constraint multi-pivot LP | **PASS** |
| **03** | `RevisedSimplex_03_OneVariableLP` | Single variable bound LP ($\max 4x \text{ s.t. } x \le 5 \implies z=20$) | **PASS** |
| **04** | `RevisedSimplex_04_OneConstraintLP` | Single constraint LP ($\max 2x_1 + 3x_2 \text{ s.t. } x_1 + x_2 \le 10 \implies z=30$) | **PASS** |
| **05** | `RevisedSimplex_05_ThreeVariableLP` | 3-var, 3-constraint LP optimum $10.0$ at $(2, 0, 1)$ | **PASS** |
| **06** | `RevisedSimplex_06_RedundantConstraintLP` | Model with redundant constraint $x_1 + x_2 \le 20$ | **PASS** |
| **07** | `RevisedSimplex_07_DegenerateLP` | Degenerate LP handling with 0 RHS basic variables | **PASS** |
| **08** | `RevisedSimplex_08_ZeroRHS` | Zero RHS constraint model | **PASS** |
| **09** | `RevisedSimplex_09_MultipleOptimalSolutions` | Flat objective facet multiple optima ($z = 5.0$) | **PASS** |
| **10** | `RevisedSimplex_10_UnboundedLP` | Unbounded ray detection ($\max x_1 + x_2 \text{ s.t. } x_1 - x_2 \le 5$) | **PASS** |
| **11** | `RevisedSimplex_11_InfeasibleLP` | Two-Phase infeasibility detection ($x_1 + x_2 \le 2$ vs $x_1 + x_2 \ge 5$) | **PASS** |
| **12** | `RevisedSimplex_12_LowerBoundVariables` | Variable lower-bound shifts ($x_1 \ge 2$) | **PASS** |
| **13** | `RevisedSimplex_13_FreeVariables` | Free variable splitting ($x_1 \in (-\infty, +\infty)$) | **PASS** |
| **14** | `RevisedSimplex_14_EqualityConstraints` | Equality constraint handling ($a_i^T x = b_i$) | **PASS** |
| **15** | `RevisedSimplex_15_GreaterThanConstraints` | Phase I artificial variable handling for $\ge$ constraints | **PASS** |
| **16** | `RevisedSimplex_16_LessThanConstraints` | Standard $\le$ slack constraint LP | **PASS** |
| **17** | `RevisedSimplex_17_ArtificialVariablePhaseI`| Dedicated Phase I artificial variable test | **PASS** |
| **18** | `RevisedSimplex_18_SmallCoefficients` | Small floating-point coefficients ($10^{-5}$) | **PASS** |
| **19** | `RevisedSimplex_19_LargeCoefficientRanges` | Large coefficient range ($1.0$ vs $10^4$) feasibility verification | **PASS** |
| **20** | `RevisedSimplex_20_NearlyDependentConstraints`| Nearly dependent constraint row stability | **PASS** |
| **21** | `RevisedSimplex_21_IterationLimitTest` | `max_iterations = 1` triggering `ITERATION_LIMIT` | **PASS** |
| **22** | `RevisedSimplex_22_NumericalFailureTest` | Singular basis matrix detection (`NUMERICAL_FAILURE`) | **PASS** |
| **23** | `RevisedSimplex_23_BasisInvariantTest` | Explicit `Basis::check_invariants()` validation | **PASS** |
| **24** | `RevisedSimplex_24_EnteringVariableTest` | Unit test for Bland's vs Dantzig pricing rules | **PASS** |
| **25** | `RevisedSimplex_25_LeavingVariableTest` | Unit test for Minimum Ratio Test leaving row selection | **PASS** |
| **26** | `RevisedSimplex_26_RatioTestEdgeCases` | Zero direction / non-positive direction unboundedness test | **PASS** |
| **27** | `RevisedSimplex_27_ObjectiveRecomputation` | Independent original model objective recomputation | **PASS** |
| **28** | `RevisedSimplex_28_FeasibilityVerification` | Independent original constraint feasibility check | **PASS** |
| **29** | `RevisedSimplex_29_PresolveRevisedPostsolvePipeline` | Integrated Presolve $\to$ Revised Simplex $\to$ Postsolve pipeline | **PASS** |
| **30** | `RevisedSimplex_30_EducationalVsRevisedComparison` | Controlled comparison: `EducationalSimplex` vs `RevisedSimplex` | **PASS** |

**Regression Status:** Test-case-based verification confirmed that Revised Simplex preserved feasibility and objective values across all **132 regression tests**.

---

## 6. Phase 6 Comparison & Performance Benchmarks

### Educational Simplex vs Revised Simplex Comparison
On controlled test models (`RevisedSimplex_30_EducationalVsRevisedComparison`):
- **Status Agreement:** 100% Match (`OPTIMAL` == `OPTIMAL`)
- **Objective Value Agreement:** Exact match ($21.333333$ vs $21.333333$, residual $< 10^{-6}$)
- **Primal Solution Agreement:** Exact match ($x = 2.666667, y = 2.666667$)

### Micro-Benchmark Result (Release Mode)
- **Model Size:** 100 Variables, 50 Constraints
- **Educational Simplex (Phase 6):** 0.9479 ms (85 Pivots)
- **Revised Simplex (Phase 7):** 61.9781 ms (85 Iterations, Dense Basis Refactorizations)
- **Note:** Revised Simplex currently utilizes dense LU refactorization per iteration (`DenseBasisSolver`). Phase 8 Sparse LU factorisation and eta-vector basis updates ($E_k$) will unlock high performance.

---

## 7. Future Scope & Limitations

1. **Phase 8 Dependency:** Basis linear system solves currently use `DenseBasisSolver`. Phase 8 will introduce Sparse LU factorisation and product-form basis updates.
2. **Advanced Pricing:** Phase 7 implements deterministic Bland's rule and Dantzig pricing. Steepest edge / Devex pricing is classified as **FUTURE**.
