# BHARATOPT — PHASE 10 REPORT
## INCREMENTAL BASIS UPDATES & REFACTORISATION CONTROL

**PROJECT:** BharatOpt — Adaptive Indigenous CPU–GPU Optimisation Solver  
**PS TITLE:** SIH 2026 PS 26119 (Target: MRPL)  
**DATE:** September 25, 2026  
**STATUS:** IMPLEMENTED & VERIFIED  
**PHASE GATE:** PASS  

---

## 1. OBJECTIVE

Phase 10 implements a modular, clean-abstraction **Incremental Basis Update Engine** using the **Product-Form of Inverse (PFI) / Eta-Vector** formulation. The primary objective is to eliminate repetitive full basis matrix factorisations during primal and dual simplex pivots, reducing unnecessary refactorisation overhead while preserving rigorous numerical correctness and stability.

---

## 2. MOTIVATION & BACKGROUND

In Phase 7 (Primal Revised Simplex), Phase 8 (Sparse LU Basis Solver), and Phase 9 (Dual Revised Simplex), full basis refactorisation was performed after *every single simplex pivot*. While mathematically sound and numerically safe, refactorising an $m \times m$ matrix at every iteration introduces significant computational overhead (e.g., $O(m^3)$ dense LU factorisation or $O(\text{nnz}(LU))$ sparse factorisation per pivot).

Phase 10 addresses this deferred requirement by introducing an incremental update layer (`BasisUpdateManager`) that maintains basis transforms incrementally via elementary column matrices (Eta vectors).

---

## 3. MATHEMATICAL FORMULATION

### 3.1 Product-Form of Inverse (PFI)
Given an initial basis $B_0$ factorised into $L_0 U_0$, after a sequence of $k$ simplex pivots resulting in bases $B_1, B_2, \dots, B_k$, the inverse basis $B_k^{-1}$ is represented as a product of elementary Eta matrices applied to $B_0^{-1}$:

$$B_k^{-1} = E_k^{-1} E_{k-1}^{-1} \dots E_1^{-1} B_0^{-1}$$

Where each $E_l^{-1}$ is an elementary column matrix corresponding to the $l$-th pivot at pivot position $p$ with direction vector $d_B = B_{l-1}^{-1} A_{\text{entering}}$:

$$E_l^{-1} = I + (\eta_l - e_p) e_p^T$$

The $p$-th column vector $\eta_l$ is defined as:

$$\eta_l [i] = \begin{cases} \frac{1}{d_B[p]} & \text{if } i = p \\ -\frac{d_B[i]}{d_B[p]} & \text{if } i \neq p \end{cases}$$

### 3.2 Primal Solve Application
To solve $B_k x = b$ for $x$:
1. Solve $B_0 x_0 = b$ using the base solver ($L_0 U_0$).
2. Sequentially apply Eta transforms for $l = 1, \dots, k$:
   $$x_l = E_l^{-1} x_{l-1}$$
   For pivot row $p$ and entry $v_p = x_{l-1}[p]$:
   $$x_l[p] = \eta_l[p] \cdot v_p$$
   $$x_l[i] = x_{l-1}[i] + \eta_l[i] \cdot v_p \quad (\forall i \neq p)$$

### 3.3 Transpose / Dual Solve Application
To solve $B_k^T y = c_B$ for $y$:
1. Apply Eta transpose transforms in **reverse order** for $l = k, k-1, \dots, 1$:
   $$w_{l-1} = E_l^{-T} w_l \quad (\text{where } w_k = c_B)$$
   For pivot row $p$:
   $$w_{l-1}[p] = \eta_l^T w_l = \sum_{i=1}^m \eta_l[i] \cdot w_l[i]$$
   $$w_{l-1}[i] = w_l[i] \quad (\forall i \neq p)$$
2. Solve $B_0^T y = w_0$ using the base solver.

---

## 4. ARCHITECTURE & MODULARITY

`BasisUpdateManager` wraps any `IBasisSolver` backend (`DenseBasisSolver` or `SparseLUBasisSolver`) using the Decorator design pattern:

```
    RevisedSimplex / DualRevisedSimplex
                    |
                    v
            IBasisSolver (Interface)
                    |
                    v
           BasisUpdateManager (Eta Chain)
                    |
                    v
    +---------------+---------------+
    |                               |
    v                               v
DenseBasisSolver           SparseLUBasisSolver
(Dense LU Reference)       (Sparse LU Markowitz)
```

Both `RevisedSimplex` and `DualRevisedSimplex` interact only with `IBasisSolver`, preventing tight coupling between the solver state machines and update mechanisms.

---

## 5. REFACTORISATION & STABILITY POLICY

Incremental updates accumulate numerical error and increase solve time per pivot as the Eta chain length grows. `BasisUpdateManager` enforces an explicit refactorisation policy:

1. **Maximum Update Count:** Triggers full refactorisation when `eta_count >= max_eta_updates` (default: 50).
2. **Pivot Stability Tolerance:** Triggers refactorisation if $|d_B[p]| < \text{pivot\_tolerance}$ (default: $10^{-10}$).
3. **Explicit Refactorisation:** Allows solvers to clear the Eta chain and rebuild $B_0$ factorisation upon phase transitions or detected instability.

---

## 6. INTEGRATION WITH REVISED SIMPLEX & DUAL REVISED SIMPLEX

### 6.1 Primal Revised Simplex Integration
- Initial factorisation executed at Phase I start.
- `add_update` called after ratio test yields leaving position `leave_pos` and entering variable `enter_var`.
- Full refactorisation invoked only if `add_update` returns `false` (threshold reached or small pivot).

### 6.2 Dual Revised Simplex Integration
- Initial factorisation executed at start.
- Direction vector $d_B = B^{-1} A_{\text{entering}}$ computed via `solve_primal`.
- `add_update` called after dual ratio test.
- Tableau row solves ($B^T w_p = e_p$) automatically leverage reverse transpose Eta chain.

---

## 7. VERIFICATION & TESTING

### 7.1 Unit Tests
Phase 10 introduces 23 dedicated unit tests (`test_basis_update.cpp`):
- `BasisUpdate_01_InitialFactorisation`: Base factorisation initialization.
- `BasisUpdate_02_SingleBasisUpdate`: Single Eta vector creation.
- `BasisUpdate_03_MultipleBasisUpdates`: Sequential Eta chain accumulation.
- `BasisUpdate_04_EtaConstruction`: Accurate $\eta_k$ coefficient computation.
- `BasisUpdate_05_EtaApplicationPrimalSolve`: Primal solve with Eta chain.
- `BasisUpdate_06_EtaApplicationTransposeSolve`: Transpose solve with Eta chain.
- `BasisUpdate_07_BasisEquivalenceAfterUpdate`: Solution equivalence vs full factorisation.
- `BasisUpdate_08_IncrementalVsFullFactorisation`: Dual Simplex equivalence.
- `BasisUpdate_09_OnePivotRevisedSimplex`: 1-pivot Primal Simplex solve.
- `BasisUpdate_10_MultiPivotRevisedSimplex`: Multi-pivot Primal Simplex solve.
- `BasisUpdate_11_DualRevisedSimplexIntegration`: Dual Simplex update integration.
- `BasisUpdate_12_RefactorisationAfterThreshold`: Refactorisation threshold trigger.
- `BasisUpdate_13_NumericalInstabilityTrigger`: Near-zero pivot detection.
- `BasisUpdate_14_NearZeroPivot`: Pivot tolerance edge case handling.
- `BasisUpdate_15_DegeneratePivot`: Degenerate pivot update handling.
- `BasisUpdate_16_RepeatedUpdatesChainLength`: Long chain length tracking.
- `BasisUpdate_17_RandomControlledBasisUpdates`: Multi-update stability.
- `BasisUpdate_18_SparseBasisLUBackend`: Integration with `SparseLUBasisSolver`.
- `BasisUpdate_19_DenseReferenceComparison`: Cross-backend equivalence.
- `BasisUpdate_20_MandatoryHandDerivedLP`: Primary benchmark LP verification ($3x + 5y \le 8, x + 2y \le 8$).
- `BasisUpdate_21_PresolveRevisedPostsolvePipeline`: Presolve $\to$ Incremental Revised Simplex $\to$ Postsolve.
- `BasisUpdate_22_PresolveDualPostsolvePipeline`: Presolve $\to$ Incremental Dual Simplex $\to$ Postsolve.
- `BasisUpdate_23_IndependentVerifier`: Verification via external feasibility checker.

### 7.2 Regression Summary
- **Phase 10 Tests:** 23/23 PASS
- **Previous Regression (Phases 0–9):** 180/180 PASS
- **Total Suite:** 203/203 PASS

---

## 8. PERFORMANCE & EXPERIMENTAL RESULTS

Benchmarked on identical hardware, compiler (LLVM-MinGW Clang 22.1.8), Release build, and tolerances for a $100 \times 50$ LP model:

| Implementation | Total Solve Time | Iterations | Refactorisations | Updates |
|---|---|---|---|---|
| **Full Refactorisation** | 69.1158 ms | 85 | 85 | 0 |
| **Incremental Basis Updates** | 12.1002 ms | 85 | 2 | 83 |

**Measured Speedup:** **~5.71x reduction** in overall solve time due to eliminating 83 expensive LU factorisations.

---

## 9. MEMORY ANALYSIS

- **Full Factorisation Memory:** $O(m^2)$ for dense LU factorisation matrix.
- **Eta Vector Memory:** $O(\text{nnz}(\eta_k))$ per update (only non-zero values stored).
- **Average Eta Memory:** ~240 bytes per update for $m = 50$, representing < 2% of full LU storage.

---

## 10. KNOWN LIMITATIONS & FUTURE WORK

1. **Markowitz Threshold Refactorisation:** Sparse LU fill-in during refactorisation can be further optimized in Phase 11.
2. **Forrest-Tomlin Updates:** PFI updates append columns to the inverse; future advanced sparse updates (e.g. Forrest-Tomlin or LU-factor updates) may be explored for very large sparse models ($m > 10,000$).

---

## 11. CONCLUSION

Phase 10 is **IMPLEMENTED & VERIFIED**. All 203 unit tests pass with zero compiler warnings, zero errors, complete mathematical integrity, and empirical performance verification.
