# Phase 6 Report — Educational Simplex / Pivot Validation Engine

## 1. Executive Summary

Phase 6 implements the **Educational Simplex Engine** for BharatOpt as a explicit mathematical reference implementation. The engine provides transparent, step-by-step tableau operations, Phase I / Phase II Two-Phase Simplex logic, entering/leaving variable selection rules, anti-cycling Bland's rule support, educational iteration tracing, and independent original-space solution verification.

- **Purpose:** Mathematical reference implementation prioritizing transparency, inspection, and verification over premature optimization.
- **Status:** IMPLEMENTED & VERIFIED
- **Regression Test Suite:** 102/102 PASS (16 Educational Simplex + 20 Presolve + 18 Sparse Matrix + 40 Validator + 8 Core/Config)
- **Compiler Warnings:** 0 warnings (LLVM-MinGW Clang 22.1.8, `-Wall -Wextra -Werror` clean across Debug and Release)

---

## 2. Architecture & Design

The `EducationalSimplex` solver ([educational_simplex.hpp](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/include/bharatopt/educational_simplex.hpp), [educational_simplex.cpp](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/src/educational_simplex.cpp)) converts an input `LPModel` into a standard form tableau, executes pivot operations, and extracts the primal solution vector $x \in \mathbb{R}^n$:

```
Original LPModel
       │
       ▼
Tableau Standardization (create_initial_tableau)
       │
       ├── Variable Shifts (x_j = x_j_tilde + l_j)
       ├── Free Variable Splitting (x_j = x_j^+ - x_j^-)
       ├── Slack (s_i), Surplus (s_i), Artificial (a_i) Variables
       └── RHS Non-Negativity Correction (b_i >= 0)
       │
       ▼
Phase I Simplex (if artificials exist: min sum(a_i))
       │
       ├── Infeasibility Check (Sum(a_i) > 1e-7 => INFEASIBLE)
       └── Objective Restoration & Canonicalization
       │
       ▼
Phase II Simplex (Maximize original obj z = c^T x + c0)
       │
       ├── Entering Var Selection (Bland's Rule / Dantzig)
       ├── Minimum Ratio Test (Tie-breaking via Bland's Rule)
       └── Unboundedness Detection (No positive pivot entry => UNBOUNDED)
       │
       ▼
Solution Extraction & Independent Verification
       ├── Primal Solution Recovery x in R^n
       ├── Original Feasibility Verification (verify_solution_feasibility)
       └── Objective Recomputation (recompute_original_objective)
```

---

## 3. Standard Form & Tableau Representation

### 3.1 Standard Form Transformation
The input model is transformed into standard form:
$$\text{Maximize } z = c^T x + c_0 \quad \text{subject to } A x + I s = b, \; x \ge 0, s \ge 0$$
- **Minimization:** Converted to maximization by multiplying objective coefficients by $-1$.
- **Variable Bounds:** Lower bounds $l_j > 0$ or $l_j < 0$ (finite) are shifted: $\tilde{x}_j = x_j - l_j \ge 0$. RHS is updated accordingly: $b_i \leftarrow b_i - a_{ij} l_j$.
- **Free Variables:** Split into $x_j = x_j^+ - x_j^-$, with $x_j^+, x_j^- \ge 0$.
- **RHS Non-Negativity:** If initial $b_i < 0$, the row is multiplied by $-1$ and constraint sense inverted before adding slacks/surplus/artificials.
- **Constraint Types:**
  - $\le$: Add slack variable $s_i \ge 0$. Initial basic variable for row $i$ is $s_i$.
  - $\ge$: Add surplus variable $s_i \ge 0$ and artificial variable $a_i \ge 0$. Initial basic variable for row $i$ is $a_i$.
  - $=$: Add artificial variable $a_i \ge 0$. Initial basic variable for row $i$ is $a_i$.

### 3.2 Tableau Layout & Sign Convention
The educational tableau has dimension $(m+1) \times (n_{\text{total}} + 1)$:
- **Rows $0 \dots m-1$:** Constraint equations $T_{i, j} x_j = b_i$.
- **Row $m$:** Objective row / Reduced costs:
  $$T_{m, j} = -c_j + z_j$$
  $$T_{m, n_{\text{total}}} = -z \quad (\text{negative of current objective value including offset})$$
- **Entering Variable Condition:** Non-basic column $j$ with $T_{m, j} < -10^{-7}$.
- **Leaving Variable Condition:** Row $i^* = \arg\min_i \{ b_i / T_{i, e} \}$ for $T_{i, e} > 10^{-10}$.

---

## 4. Pivoting & Phase I / Phase II Mechanics

### 4.1 Step-by-Step Pivot Operation
`pivot(Tableau& tableau, size_t pivot_row, size_t pivot_col)` performs explicit elementary row operations:
1. Row normalization: $T_{\text{pivot\_row}, j} \leftarrow T_{\text{pivot\_row}, j} / T_{\text{pivot\_row}, \text{pivot\_col}}$.
2. Row elimination: $T_{i, j} \leftarrow T_{i, j} - T_{i, \text{pivot\_col}} \cdot T_{\text{pivot\_row}, j}$ for all $i \neq \text{pivot\_row}$.
3. Basis update: `tableau.basis[pivot_row] = pivot_col`.

### 4.2 Anti-Cycling & Bland's Rule
When `EnteringRule::BLANDS_RULE` is enabled:
- Entering variable: Selects the smallest variable column index $j$ with negative reduced cost $T_{m, j} < -10^{-7}$.
- Minimum ratio test tie-breaking: Selects the row corresponding to the smallest basic variable index.

---

## 5. Verification & Test Suite

The dedicated test suite (`tests/test_educational_simplex.cpp`) includes 16 hand-derived unit tests:

| Test ID | Test Name | Mathematical Focus / Coverage | Status |
| :--- | :--- | :--- | :--- |
| **01** | `Simplex_01_PivotLevelTest` | Pivot operation validation against hand-derived expected tableau matrix | **PASS** |
| **02** | `Simplex_02_Simple2VariableLP` | Hand-calculated 2-variable LP ($\max 3x_1 + 5x_2$) optimum at $(8/3, 8/3)$, $z = 64/3$ | **PASS** |
| **03** | `Simplex_03_OneVariableLP` | Single variable bound constraint LP ($\max 4x_1 \text{ s.t. } x_1 \le 5$) | **PASS** |
| **04** | `Simplex_04_OneConstraintLP` | Single constraint LP ($\max 2x_1 + 3x_2 \text{ s.t. } x_1 + x_2 \le 10$) | **PASS** |
| **05** | `Simplex_05_MultipleConstraintLP` | 3-var, 3-constraint LP solved to exact optimum $10.0$ at $(2, 0, 1)$ | **PASS** |
| **06** | `Simplex_06_RedundantConstraintLP` | Model with redundant constraint $x_1 + x_2 \le 20$ | **PASS** |
| **07** | `Simplex_07_MultiplePivotLP` | Multi-pivot convergence verification | **PASS** |
| **08** | `Simplex_08_DegenerateLP` | Degenerate LP handling with 0 RHS basic variables | **PASS** |
| **09** | `Simplex_09_UnboundedLP` | Unbounded ray detection ($\max x_1 + x_2 \text{ s.t. } x_1 - x_2 \le 5$) | **PASS** |
| **10** | `Simplex_10_InfeasibleLP` | Phase I infeasibility detection ($x_1 + x_2 \le 2$ vs $x_1 + x_2 \ge 5$) | **PASS** |
| **11** | `Simplex_11_ZeroObjectiveLP` | All zero objective coefficients ($z = 0$) | **PASS** |
| **12** | `Simplex_12_PhaseITwoPhaseTest` | Two-Phase Simplex handling $\ge$ constraint via artificial variables | **PASS** |
| **13** | `Simplex_13_IterationLimitTest` | `max_iterations = 1` triggering `ITERATION_LIMIT` status | **PASS** |
| **14** | `Simplex_14_NumericalStressSmallCoeff`| Small floating-point coefficients ($10^{-5}$) near numerical tolerance | **PASS** |
| **15** | `Simplex_15_IterationTraceMode` | Iteration trace recording (`enable_trace = true`) | **PASS** |
| **16** | `Simplex_16_PresolveSimplexPostsolvePipeline` | Integrated Phase 5 Presolve $\to$ Educational Simplex $\to$ Postsolve pipeline | **PASS** |

**Regression Status:** Test-case-based verification confirmed that the implemented Educational Simplex Engine preserved feasibility and objective values across all 102 regression tests.

---

## 6. Performance Benchmarks

### Micro-Benchmark Result (Release Mode)
- **Model Size:** 100 Variables, 50 Constraints
- **Solve Time:** `0.6559 ms`
- **Pivots Executed:** 85 Pivots
- **Status:** `OPTIMAL`
