# Phase 16 Technical Report: MILP Model Foundation & LP Relaxation

**Project:** BHARATOPT — Adaptive Indigenous CPU–GPU Optimisation Solver  
**Phase:** 16 — MILP Foundation: Integer Variables + LP Relaxation  
**Date:** September 25, 2026  
**Status:** IMPLEMENTED & VERIFIED  

---

## 1. Objective

Phase 16 extends BHARATOPT from a pure Linear Programming (LP) solver into a mathematically sound Mixed Integer Linear Programming (MILP) model foundation. This phase establishes:
1. Formal integer ($\mathbb{Z}$) and binary ($\{0,1\}$) variable semantics.
2. Deterministic model classification (`ModelType::LP`, `ModelType::MILP`, `ModelType::UNSUPPORTED_MODEL`).
3. Explicit, immutable LP relaxation extraction preserving all problem structure (objective coefficients, sense, offset, constraints, RHS, bounds, variable ordering).
4. Validation of binary ($0 \le x_j \le 1$) and integer domains.
5. Independent integer feasibility checking with explicit integrality tolerance $\epsilon_{\text{integer}} = 10^{-5}$.
6. Independent solution verification for continuous relaxation solutions and integer-feasible solutions.

---

## 2. Existing Architecture Reused

In strict compliance with architectural directives, zero duplicate model primitives were built. Phase 16 reuses:
- `LPModel`, `Variable`, `Constraint`, `ObjectiveSense`, `VariableType`
- `ModelValidator` & `ValidationResult`
- `PresolveEngine` & `Postsolve`
- `SparseMatrix` & Sparse LU factorisation
- `DualRevisedSimplex` & `RevisedSimplex` solvers
- Independent solution verifier

---

## 3. Supported MILP Semantics

BHARATOPT supports models of the form:
$$\min/\max \quad c^T x + c_0$$
$$\text{subject to} \quad A x \le / = / \ge b, \quad l \le x \le u$$
$$\text{with } x_j \in \mathbb{R} \text{ (CONTINUOUS)}, \quad x_j \in \mathbb{Z} \text{ (INTEGER)}, \quad x_j \in \{0, 1\} \text{ (BINARY)}.$$

- **LP Classification:** All variables are `CONTINUOUS`.
- **MILP Classification:** At least one variable is `INTEGER` or `BINARY` and all variable/constraint data is structurally valid.
- **Unsupported:** Quadratic, non-linear, or invalid domain structures.

---

## 4. LP Relaxation Transformation & Immutability

The `LPRelaxationEngine` transforms a MILP model into its continuous LP relaxation:
- Binary variables ($x_j \in \{0, 1\}$) are relaxed to continuous bounds $0 \le x_j \le 1$.
- Integer variables ($x_j \in \mathbb{Z}$) retain their continuous numerical bounds $l_j \le x_j \le u_j$.
- Objective coefficients, sense, offset, constraint terms, RHS, and range bounds are copied identically.
- **Immutability Guarantee:** The original `LPModel` remains completely unmodified during relaxation extraction, solving, and integer verification. Modifying the relaxation model has zero effect on the original MILP instance.

---

## 5. Integer Feasibility Verification

The `IntegerFeasibilityChecker` independently verifies solution vectors:
- For `INTEGER`: $|x_j - \text{round}(x_j)| \le \epsilon_{\text{integer}} = 10^{-5}$.
- For `BINARY`: $\min(|x_j - 0|, |x_j - 1|) \le \epsilon_{\text{integer}} = 10^{-5}$.
- Also checks bound feasibility ($l_j - 10^{-4} \le x_j \le u_j + 10^{-4}$) and constraint residuals ($||Ax - b||_\infty \le 10^{-4}$).

---

## 6. Solution Status Classification

Results are classified under explicit categories:
- `LP_RELAXATION_OPTIMAL`: Relaxation solved to continuous optimality; integer variables remain fractional.
- `INTEGER_FEASIBLE_SOLUTION`: Relaxation solution satisfies all integrality, bound, and constraint tolerances. *(Note: Reported as an LP-relaxation solution that happens to be integer-feasible; global MILP optimality proof is not claimed at this stage).*
- `INTEGER_INFEASIBLE_SOLUTION`: Solution vector violates integrality or integer bounds.
- `NUMERICAL_FAILURE`: LP solver failed to converge.
- `UNSUPPORTED_MODEL`: Model validation error or unsupported variable type.

---

## 7. Mandatory Hand-Derived Test Results

1. **Hand Case 1 ($\max 3x + 5y \text{ s.t. } 2x + y \le 8, x + 2y \le 8, x, y \ge 0, \text{integer}$):**
   - Continuous LP relaxation optimum: $x^* = 8/3 \approx 2.6667, y^* = 8/3 \approx 2.6667, z^* = 64/3 \approx 21.3333$.
   - Integer Feasibility Checker: Correctly flags 2 fractional variables ($\text{violating\_count} = 2, \text{max\_viol} = 0.3333$).
   - Solution Status: `LP_RELAXATION_OPTIMAL`.

2. **Hand Case 2 ($x + y \le 1, x, y \text{ binary}$):**
   - LP Relaxation solved to objective $z^* = 1.0$.
   - Independent Integer Verifier validates integrality and constraint feasibility.

3. **Hand Case 3 ($x + 2y \ge 3, x, y \ge 0, \text{integer}$):**
   - LP Relaxation optimum: $x^* = 3.0, y^* = 0.0, z^* = 3.0$.
   - Integer Feasibility Checker: Validates that solution is integer-feasible ($\text{violating\_count} = 0$).
   - Solution Status: `INTEGER_FEASIBLE_SOLUTION`.

---

## 8. Regression & Performance Results

- **Baseline Regression (Phases 0–15):** 278 / 278 PASS
- **Phase 16 Unit Tests:** 21 / 21 PASS
- **Total Regression:** 299 / 299 PASS
- **Build Verification:** 0 errors, 0 warnings (Debug & Release)
- **Overhead Measurements:**
  - Model Classification: $0.002\text{ ms}$
  - LP Relaxation Extraction: $0.008\text{ ms}$
  - Integer Feasibility Checking: $0.005\text{ ms}$
  - Total Phase 16 Overhead: $< 0.02\text{ ms}$

---

## 9. Explicit Non-Goals & What Is NOT Implemented

The following components are explicitly reserved for Phase 17 (Branch-and-Bound):
- Branch-and-Bound tree search
- Branch-and-Cut / Cutting planes (Gomory, MIR, Cover cuts)
- Node selection strategies (DFS, Best-Bound, Estimate)
- Incumbent management & Pruning
- Primal heuristics
- Parallel / GPU MILP search
