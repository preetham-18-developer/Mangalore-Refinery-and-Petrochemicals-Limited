# BharatOpt — Testing Strategy & Quality Assurance Framework

**Document Version:** 1.0.0  
**Date:** September 25, 2026  
**Status:** Approved Quality Assurance Standard  

---

## 1. Multi-Tiered Testing Hierarchy

Testing in BharatOpt is structured across five distinct tiers to guarantee numerical stability, mathematical equivalence, and software reliability:

```
+-------------------------------------------------------+
| TIER 5: NUMERICAL STRESS & EDGE CASE TESTING          |
+-------------------------------------------------------+
                           ^
                           |
+-------------------------------------------------------+
| TIER 4: REGRESSION TEST SUITE                         |
+-------------------------------------------------------+
                           ^
                           |
+-------------------------------------------------------+
| TIER 3: REFERENCE ORACLE COMPARISON (VS HIGHS)        |
+-------------------------------------------------------+
                           ^
                           |
+-------------------------------------------------------+
| TIER 2: MATHEMATICAL VALIDATION TESTS                 |
+-------------------------------------------------------+
                           ^
                           |
+-------------------------------------------------------+
| TIER 1: UNIT & INTEGRATION TESTS (CATCH2 / GTEST)     |
+-------------------------------------------------------+
```

---

## 2. Detailed Testing Tier Specifications

### Tier 1: Unit & Integration Testing
- **Scope:** Component-level validation (Sparse Matrix conversions, vector operations, model construction, presolve transformations).
- **Execution:** Automated C++ unit tests compiled with Debug assertions and AddressSanitizer/UBSanitizer where supported.

### Tier 2: Mathematical Validation Testing
- **Scope:** Internal mathematical equivalence verification.
- **Examples:**
  - Sparse vs Dense SpMV comparison ($y_{\text{sparse}} \text{ vs } y_{\text{dense}}$) across random vectors $x$, verifying $\text{max\_abs\_err} < 10^{-12}$.
  - Presolve solution recovery: solving original LP model vs solving presolved model, applying `Postsolve` mapping, and verifying primal feasibility and exact objective match.

### Tier 3: Reference Oracle Comparison
- **Scope:** Solution correctness comparison against independent solver (HiGHS).
- **Tolerances:**
  - Relative objective tolerance: $\frac{|z_{\text{BharatOpt}} - z_{\text{Oracle}}|}{\max(1, |z_{\text{Oracle}}|)} \le 10^{-6}$.
  - Primal feasibility residual: $\|Ax - b\|_\infty \le 10^{-6}$.
  - Integrality tolerance (MILP): $|x_j - \lfloor x_j \rceil| \le 10^{-5}, \forall j \in \mathcal{I}$.

### Tier 4: Automated Regression Suite
- **Scope:** Preventing feature regressions.
- **Rule:** Every fixed bug or numerical edge case must be converted into a permanent regression test added to `tests/regression/`. All regression tests must execute and pass before any Phase Gate approval.

### Tier 5: Numerical Stress Testing
- **Scope:** Boundary conditions and ill-conditioned systems.
- **Scenarios:**
  - Ill-conditioned basis matrices (condition number $\kappa(B) > 10^8$).
  - Degenerate LPs (multiple zero reduced costs, stalling risk).
  - Unbounded models ($c^T x \to -\infty$).
  - Infeasible models ($Ax \le b$ has no solution space).
  - Empty or single-variable models.
  - Huge matrix coefficients ($10^9$) and tiny non-zeros ($10^{-12}$).

---

## 3. Standardized Step Gate Template

At the conclusion of every development phase, an explicit phase gate report artifact **MUST** be generated at `docs/gates/phase-X-gate.md` using the exact standard format:

```markdown
PHASE: Phase X — [Phase Title]
STATUS: [COMPLETE / IN PROGRESS / FAILED]

FUNCTIONAL CORRECTNESS: PASS / FAIL
MATHEMATICAL CORRECTNESS: PASS / FAIL
NUMERICAL CORRECTNESS: PASS / FAIL
REFERENCE COMPARISON: PASS / FAIL
PERFORMANCE: PASS / FAIL / NOT APPLICABLE
MEMORY: PASS / FAIL / NOT APPLICABLE
EDGE CASES: PASS / FAIL
REGRESSION: PASS / FAIL

KNOWN ISSUES:
- None / [List of tracked issues]

BENCHMARK RESULTS:
- [Summary of runtime, memory, and objective verification data]

DECISION:
GO / STOP AND FIX
```

---

## 4. Failure Protocol

If any test or gate check returns **FAIL**:

1. **STOP IMMEDIATELY.** Do not proceed to the next phase.
2. Isolate the smallest reproducible failing test case.
3. Diagnose the mathematical or algorithmic root cause (do NOT apply superficial symptom patches or swallow exceptions).
4. Apply the fix and rerun the entire test suite (Tiers 1–5).
5. Only when all tests pass may the Phase Gate decision be set to **GO**.

---
*Maintained as the testing strategy for BharatOpt.*
