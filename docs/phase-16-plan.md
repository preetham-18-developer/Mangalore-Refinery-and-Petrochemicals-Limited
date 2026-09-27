# Phase 16 Architectural Plan: MILP Model Foundation & LP Relaxation

**Project:** BHARATOPT — Adaptive Indigenous CPU–GPU Optimisation Solver  
**Phase:** 16 — MILP Foundation: Integer Variables + LP Relaxation  
**Date:** September 25, 2026  

---

## 1. Current Codebase Capabilities

Inspection of `include/bharatopt/lp_model.hpp`, `src/lp_model.cpp`, and `src/model_validator.cpp` reveals the following pre-existing primitives:
- `VariableType` enum (`CONTINUOUS`, `INTEGER`, `BINARY`) is defined in `lp_model.hpp`.
- `struct Variable` contains `VariableType type` field and bounds.
- `ModelValidator` checks for basic invalid binary bounds ($l_j < 0$ or $u_j > 1$).

---

## 2. Missing Capabilities & Required Enhancements

1. **Formal Model Classification (`ModelType`):**
   - Explicit `classify_model(const LPModel&)` function returning `ModelType::LP`, `ModelType::MILP`, or `ModelType::UNSUPPORTED_MODEL`.
   - Returns `LP` if all variables are `CONTINUOUS`.
   - Returns `MILP` if at least one variable is `INTEGER` or `BINARY` and all model data is valid.
   - Returns `UNSUPPORTED_MODEL` if invalid variable types or unsupported structures are present.

2. **Binary & Integer Validation Rules:**
   - Binary variables MUST satisfy $0 \le x_j \le 1$. If $l_j < 0$ or $u_j > 1$, `ModelValidator` generates a validation `ERROR`.
   - Non-integer bounds on `INTEGER` variables (e.g., $2.5 \le x_j \le 10.0$): Documented policy tightens bounds ($l_j \leftarrow \lceil l_j \rceil = 3$, $u_j \leftarrow \lfloor u_j \rfloor = 10$) with a diagnostic warning, preserving mathematical soundness.

3. **LP Relaxation Engine (`LPRelaxationEngine`):**
   - Creates an independent, immutable `LPModel` relaxation where all `INTEGER` and `BINARY` variables are transformed to `CONTINUOUS`.
   - Binary bounds become continuous $0 \le x_j \le 1$.
   - Integer bounds retain their continuous range $l_j \le x_j \le u_j$.
   - All objective coefficients, objective sense, objective offset, constraint coefficients, senses, RHS, ranges, variable names, and variable indices are strictly preserved.
   - Provides explicit `RelaxationMapping` (bidirectional index mapping) and enforces model immutability tests (modifying relaxation does not alter original MILP).

4. **Independent Integer Feasibility Checker (`IntegerFeasibilityChecker`):**
   - Given a continuous LP relaxation solution vector $x$, checks integrality for all integer/binary variables using an explicit tolerance $\epsilon_{\text{integer}} = 10^{-5}$.
   - For `INTEGER`: $|x_j - \text{round}(x_j)| \le \epsilon_{\text{integer}}$.
   - For `BINARY`: $|x_j| \le \epsilon_{\text{integer}}$ or $|x_j - 1.0| \le \epsilon_{\text{integer}}$.
   - Returns detailed diagnostics: `is_integer_feasible`, `max_integrality_violation`, `violating_variable_count`, `max_bound_violation`, `max_constraint_residual`.

5. **MILP Solution Classification & Verifier (`MilpSolutionStatus`):**
   - Solution status categories: `LP_RELAXATION_OPTIMAL`, `INTEGER_FEASIBLE_SOLUTION`, `INTEGER_INFEASIBLE_SOLUTION`, `NUMERICAL_FAILURE`, `UNSUPPORTED_MODEL`.
   - Global MILP optimality is explicitly NOT claimed at this stage (reserved for future Branch-and-Bound in Phase 17).
   - Solves relaxation via existing verified LP solvers (`RevisedSimplex`, `DualRevisedSimplex`, `CPUFirstOrderSolver`, `GPUFirstOrderSolver`).

---

## 3. Files to Modify and Add

### Files to Add:
- `include/bharatopt/milp_foundation.hpp`: Declarations for `ModelType`, `LPRelaxationEngine`, `RelaxationMapping`, `IntegerFeasibilityChecker`, `MilpRelaxationResult`, `MilpSolutionStatus`.
- `src/milp_foundation.cpp`: Implementations of relaxation extraction, classification, integer checking, and relaxation solving.
- `tests/test_milp_foundation.cpp`: Comprehensive unit test suite (Categories A-G + Mandatory Hand-Derived Cases).
- `docs/phase-16-report.md`: Technical documentation and results.
- `docs/gates/phase-16-gate.md`: Quality gate evaluation.

### Files to Modify:
- `include/bharatopt/model_validator.hpp` & `src/model_validator.cpp`: Strengthen binary bound error enforcement and integer bound policy.
- `CMakeLists.txt` & `tests/CMakeLists.txt`: Add new source and test files.

---

## 4. Mathematical Invariants & Test Strategy

- **Immutability Invariant:** Original `LPModel` instance must remain completely unchanged throughout classification, relaxation, solving, and integer verification.
- **Data Invariant:** $\text{Relaxed}(A, b, c, \text{sense}, \text{offset}) \equiv \text{Original}(A, b, c, \text{sense}, \text{offset})$.
- **Verification Strategy:** 100% test coverage across classification, bound validation, relaxation mapping, integer feasibility tolerance bounds, numerical edge cases, and mandatory hand-derived MILP models.
