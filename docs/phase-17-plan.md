# Phase 17 Architectural Plan: CPU Branch-and-Bound Engine for MILP

**Project:** BHARATOPT — Adaptive Indigenous CPU–GPU Optimisation Solver  
**Phase:** 17 — CPU Branch-and-Bound Engine for MILP  
**Date:** September 25, 2026  

---

## 1. Architecture Findings & Reused Components

In strict compliance with project directives, zero duplicate model, validation, or LP solver infrastructure will be created. Phase 17 reuses:
- `LPModel`, `Variable`, `Constraint`, `ObjectiveSense`, `VariableType` from Phase 2.
- `ModelValidator` & `ValidationResult` from Phase 3.
- `DualRevisedSimplex` & `RevisedSimplex` solvers from Phases 7–9.
- `MilpFoundation` (Model classification, LP relaxation extraction, integer feasibility verification) from Phase 16.
- Independent solution verifier.

---

## 2. New Components Required

1. **`BnBNode` Data Structure:**
   - Stores node ID, parent node ID, depth, accumulated bound changes, current node lower/upper bounds, LP relaxation objective bound, LP solution vector, branching variable index, branching value, and explicit status (`BnBNodeStatus`).

2. **`BranchAndBoundEngine` Class:**
   - Orchestrates the B&B search tree.
   - Enforces original `LPModel` immutability (node-specific bounds applied to independent LP relaxation copies).
   - Manages incumbent integer-feasible solutions.
   - Implements deterministic **BEST-BOUND** node selection.
   - Implements deterministic **MOST FRACTIONAL** variable selection.
   - Handles bound pruning, infeasibility pruning, node count limits, and numerical failure handling.
   - Conducts independent verification of final MILP solution.

3. **`BnBConfig`, `BnBTelemetry`, `BnBResult`:**
   - Provides configuration limits (`max_nodes`, `time_limit_ms`, `integrality_tolerance`, `objective_tolerance`).
   - Tracks precise execution telemetry (nodes created, processed, pruned by infeasibility, pruned by bound, integer-feasible nodes, max tree depth, best open bound).

---

## 3. Mathematical Invariants

- **Immutability Invariant:** The original `LPModel` instance is NEVER modified during B&B tree traversal.
- **Bounding Invariant:**
  - For MAXIMIZATION: Node LP relaxation objective $z_{\text{LP}}$ represents an upper bound on all integer solutions in the subtree ($z_{\text{MILP}} \le z_{\text{LP}}$).
  - For MINIMIZATION: Node LP relaxation objective $z_{\text{LP}}$ represents a lower bound on all integer solutions in the subtree ($z_{\text{MILP}} \ge z_{\text{LP}}$).
- **Pruning Invariant:**
  - MAXIMIZATION: Node pruned if $z_{\text{LP}} \le z_{\text{incumbent}} + \text{obj\_tol}$.
  - MINIMIZATION: Node pruned if $z_{\text{LP}} \ge z_{\text{incumbent}} - \text{obj\_tol}$.
- **Branching Invariant:** Two child nodes partitioned by $x_j \le \lfloor v \rfloor$ (left) and $x_j \ge \lceil v \rceil$ (right) for fractional $v = x_j^*$.

---

## 4. Test Strategy

1. **Hand Case 1 (Binary Relaxed Constraint):** Maximize $x + y$ s.t. $2x + 2y \le 3, x,y \in \{0,1\}$ (LP relaxation $1.5 \implies$ MILP optimum $1.0$).
2. **Hand Case 2 (2D Integer Simplex):** Maximize $x + y$ s.t. $2x + y \le 4, x + 2y \le 4, x,y \ge 0, \text{integer}$ (LP relaxation $8/3 \implies$ MILP optimum $2.0$).
3. **Hand Case 3 (Phase 16 Model):** Maximize $3x + 5y$ s.t. $2x + y \le 8, x + 2y \le 8, x,y \ge 0, \text{integer}$ (LP relaxation $64/3 \implies$ MILP optimum $20.0$ at $(0,4)$).
4. **Hand Case 4 (Root Feasible MILP):** LP relaxation already integer-feasible $\implies$ 0 branching nodes required.
5. **Hand Case 5 (Infeasible MILP):** Contradictory bounds/constraints $\implies$ `BnBSolverStatus::INFEASIBLE`.
6. **Hand Case 6 (Minimization MILP):** Verify lower-bounding and minimization pruning.
7. **Edge Cases & Limits:** Node limit reached (`LIMIT_REACHED`), original model immutability test, tight bounds, duplicate restrictions.

---

## 5. Known Risks & Scope Boundaries

- **Scope Boundary:** No cutting planes, Branch-and-Cut, strong branching, heuristics, parallel B&B, or GPU search are included in Phase 17.
- **Risk Mitigation:** Strict numerical tolerances ($10^{-5}$ integrality, $10^{-7}$ objective, $10^{-4}$ residual) prevent false pruning or floating-point loops.
