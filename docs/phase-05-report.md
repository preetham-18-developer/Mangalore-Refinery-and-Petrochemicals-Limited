# Phase 5 Report — Presolve Engine

## 1. Executive Summary

Phase 5 delivers the **Presolve Engine** for BharatOpt, designed to reduce linear programming (LP) and mixed-integer linear programming (MILP) model size prior to numerical solving. The engine inspects structural properties and variable bounds, eliminating redundant variables and constraints, tightening bounds, and detecting infeasibility or unboundedness early.

**Key Rule:** *Presolve must preserve solution correctness.* Every forward reduction maintains a deterministic `TransformationRecord` enabling exact postsolve mapping back to the original model space $\mathbb{R}^n$.

- **Status:** IMPLEMENTED & VERIFIED
- **Regression Test Suite:** 86/86 PASS (20 Presolve + 18 Sparse Matrix + 40 Validator + 8 Core/Config)
- **Compiler Warnings:** 0 warnings (LLVM-MinGW Clang 22.1.8, `-Wall -Wextra -Werror` clean)

---

## 2. Presolve Architecture

The Presolve Engine sits between the `ModelValidator` and the future `NumericalSolver`:

```
Original LPModel
       │
       ▼
 ModelValidator
       │
       ▼
 PresolveEngine ───(Iterative Reduction Loop)───► PresolveResult
                                                      ├── Reduced LPModel
                                                      ├── PresolveStatistics
                                                      ├── PresolveStatus
                                                      └── Postsolve
                                                             │
                                                      (Solver Execution)
                                                             │
                                                             ▼
                                                    Reduced Solution x'
                                                             │
                                                             ▼
                                                    Postsolve Mapping
                                                             │
                                                             ▼
                                                   Original Solution x
```

### Core Components

1. **`PresolveEngine`**: Executes a controlled reduction loop over the `LPModel` until convergence (`max_passes`, default = 100) or an early termination condition (`INFEASIBLE` / `UNBOUNDED_IF_DETECTABLE`).
2. **`TransformationRecord`**: Encapsulates individual reduction steps (fixed variables, removed empty constraints, singleton row bound tightenings) with parameters for exact reversal.
3. **`Postsolve`**: Manages the reconstruction of original variables $x \in \mathbb{R}^n$ from reduced variables $x' \in \mathbb{R}^{n'}$, verifies original feasibility, and recomputes the original objective value $c^T x + c_0$.
4. **`PresolveResult`**: Contains the resulting reduced `LPModel`, `PresolveStatistics`, `PresolveStatus`, and the `Postsolve` instance.

---

## 3. Implemented Presolve Rules & Mathematical Rationale

Each presolve rule is mathematically justified and mapped to exact postsolve steps:

### 3.1 Fixed Variable Elimination
- **Condition:** $l_j = u_j = v_j$ (within tolerance $\varepsilon = 10^{-7}$).
- **Forward Transformation:**
  - Update objective offset: $c_0 \leftarrow c_0 + c_j \cdot v_j$.
  - Substitute $a_{ij} x_j = a_{ij} v_j$ into constraint $i$, updating RHS: $b_i \leftarrow b_i - a_{ij} v_j$.
  - Remove column $j$ from the active variable set.
- **Postsolve Mapping:** Set $x_j = v_j$ in the original solution vector.

### 3.2 Empty Row Elimination & Infeasibility Check
- **Condition:** Constraint $i$ has 0 active variable coefficients ($\sum_{j} |a_{ij}| = 0$).
- **Feasibility Verification:**
  - $\le$ sense: If $0 > b_i + \varepsilon$, mark status `PRESOLVE_INFEASIBLE`.
  - $\ge$ sense: If $0 < b_i - \varepsilon$, mark status `PRESOLVE_INFEASIBLE`.
  - $=$ sense: If $|0 - b_i| > \varepsilon$, mark status `PRESOLVE_INFEASIBLE`.
  - Ranged: If $0 < b_i^L - \varepsilon$ or $0 > b_i^U + \varepsilon$, mark status `PRESOLVE_INFEASIBLE`.
- **Forward Transformation:** If feasible, remove the redundant empty row from the model.
- **Postsolve Mapping:** No variable reconstruction needed.

### 3.3 Empty Column Processing & Unboundedness Check
- **Condition:** Variable $j$ has no constraint coefficients ($\sum_{i} |a_{ij}| = 0$).
- **Analysis:**
  - **Objective coefficient $c_j > 0$ (minimization):** Optimal value occurs at lower bound $l_j$.
    - If $l_j > -\infty$: Fix $x_j = l_j$, record transformation, update objective offset $c_0 \leftarrow c_0 + c_j l_j$.
    - If $l_j = -\infty$: Mark status `PRESOLVE_UNBOUNDED_IF_DETECTABLE`.
  - **Objective coefficient $c_j < 0$ (minimization):** Optimal value occurs at upper bound $u_j$.
    - If $u_j < +\infty$: Fix $x_j = u_j$, record transformation, update objective offset $c_0 \leftarrow c_0 + c_j u_j$.
    - If $u_j = +\infty$: Mark status `PRESOLVE_UNBOUNDED_IF_DETECTABLE`.
  - **Objective coefficient $c_j = 0$:** Objective is invariant to $x_j$. Fix $x_j$ at any finite value within $[l_j, u_j]$ (defaults to $0.0$ bounded by $[l_j, u_j]$).

### 3.4 Singleton Row Processing & Bound Tightening
- **Condition:** Constraint $i$ contains exactly one non-zero coefficient $a_{ij} x_j$.
- **Bound Tightening:**
  - $a_{ij} x_j \le b_i \implies x_j \le \frac{b_i}{a_{ij}}$ (if $a_{ij} > 0$) or $x_j \ge \frac{b_i}{a_{ij}}$ (if $a_{ij} < 0$).
  - $a_{ij} x_j \ge b_i \implies x_j \ge \frac{b_i}{a_{ij}}$ (if $a_{ij} > 0$) or $x_j \le \frac{b_i}{a_{ij}}$ (if $a_{ij} < 0$).
  - $a_{ij} x_j = b_i \implies x_j = \frac{b_i}{a_{ij}}$ (fixes lower and upper bound to $\frac{b_i}{a_{ij}}$).
- **Contradictory Check:** If tightened lower bound $l_j' > u_j' + \varepsilon$, mark status `PRESOLVE_INFEASIBLE`.
- **Forward Transformation:** Update variable bounds $(l_j, u_j)$ and remove the singleton constraint.

### 3.5 Redundant Constraint Elimination
- **Condition:** Constraint bounds are implied entirely by variable bounds.
- **Forward Transformation:** Safely remove constraint $i$.

---

## 4. Postsolve & Correctness Verification

The fundamental critical requirement of Phase 5 is:
$$\text{Presolve}(M) \to M' \quad \implies \quad \text{Solve}(M') \to x' \quad \implies \quad \text{Postsolve}(x') \to x \quad \implies \quad x \text{ satisfies } M$$

### Postsolve Guarantee
1. **Feasibility Verification:** `verify_original_feasibility(orig_x)` evaluates $A x \le b$ (and $\ge, =$, ranged) against all original constraints in $M$.
2. **Objective Invariance:** `compute_original_objective(orig_x)` verifies that:
   $$\left| \left(c^T x + c_0\right)_{\text{original}} - \left({c'}^T x' + c'_0\right)_{\text{reduced}} \right| < 10^{-7}$$

---

## 5. Verification & Test Suite

The dedicated presolve test suite (`tests/test_presolve.cpp`) includes 20 comprehensive unit tests:

| Test ID | Test Case Name | Coverage Description | Status |
| :--- | :--- | :--- | :--- |
| **01** | `Presolve_01_NoOpModel` | Model with no reductions returns `NO_CHANGE` | **PASS** |
| **02** | `Presolve_02_FixedVariableElimination` | Single fixed variable substitution and postsolve | **PASS** |
| **03** | `Presolve_03_MultipleFixedVariables` | Simultaneous fixed variable eliminations | **PASS** |
| **04** | `Presolve_04_EmptyRowRedundant` | Safe removal of empty $0 \le 10$ row | **PASS** |
| **05** | `Presolve_05_EmptyRowInfeasible` | Infeasibility detection on empty $0 \ge 5$ row | **PASS** |
| **06** | `Presolve_06_EmptyColumnBounded` | Empty column fixed at optimal bound | **PASS** |
| **07** | `Presolve_07_EmptyColumnUnbounded` | Detection of unconstrained free empty column | **PASS** |
| **08** | `Presolve_08_SingletonRow` | Singleton row bound tightening and removal | **PASS** |
| **09** | `Presolve_09_BoundTightening` | Bound tightening propagation | **PASS** |
| **10** | `Presolve_10_ContradictoryBounds` | Infeasibility detection on contradictory bounds | **PASS** |
| **11** | `Presolve_11_RedundantConstraint` | Redundant constraint detection and removal | **PASS** |
| **12** | `Presolve_12_MultipleSimultaneousReductions` | Fixed var + empty row in single model | **PASS** |
| **13** | `Presolve_13_ObjectiveConstantPreservation` | Objective offset accumulation check | **PASS** |
| **14** | `Presolve_14_VariableMapping` | Reduced model variable index re-mapping | **PASS** |
| **15** | `Presolve_15_PostsolveReconstruction` | Exact reconstruction of original vector $x$ | **PASS** |
| **16** | `Presolve_16_OriginalFeasibilityAfterPostsolve`| Original constraint feasibility verification | **PASS** |
| **17** | `Presolve_17_OriginalObjectiveAfterPostsolve` | Original objective value matching | **PASS** |
| **18** | `Presolve_18_ModelNoReductionsPossible` | Invariant model return status | **PASS** |
| **19** | `Presolve_19_MixedReductionModel` | Interleaved multi-pass reduction loop | **PASS** |
| **20** | `Presolve_20_NumericalEdgeCases` | Near-zero bound gap handling within tolerance | **PASS** |

**Regression Status:** All 86 unit tests across Phase 1, Phase 2, Phase 3, Phase 4, and Phase 5 pass cleanly.

---

## 6. Performance Benchmarks

### Micro-Benchmark Result (Release Mode)
- **Model Size:** 5,000 Variables, 3,000 Constraints (Synthetically constructed with fixed vars & empty cols)
- **Presolve Execution Time:** `23.70 ms`
- **Variables Removed:** 1,500 (30.0% reduction)
- **Status:** `PRESOLVE_SUCCESS`

---

## 7. Future Scope (Documented Non-Scope)

As per Phase 5 design specifications, the following advanced techniques are classified as **FUTURE** to ensure absolute mathematical safety before numerical solvers are integrated:
- Advanced variable-variable substitution ($x_i = \alpha x_j + \beta$)
- Dual presolve and implied bound propagation through multi-row linear combinations
- Aggressive numerical scaling (equilibrate matrix norms)
