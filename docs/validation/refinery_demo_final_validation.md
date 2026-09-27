# Synthetic Refinery Production-Planning Final Validation

> **Notice**: This model is a **synthetic refinery production-planning case study** for algorithm testing and demonstration purposes. It does not represent actual confidential MRPL operational refinery dataset files.

---

## 1. Mathematical Formulation & Model Specifications
- **Model Identifier**: `refinery_demo` (`bharatopt_refinery_demo(1).mps` / `bharatopt_refinery_demo.mps`)
- **Constraints ($M$)**: 5
- **Variables ($N$)**: 3 (`GASOLINE`, `DIESEL`, `NAPHTHA`)
- **Matrix Non-Zeros ($\text{NNZ}$)**: 11
- **Sparsity Density**: 73.33%
- **Objective Sense**: `MAXIMIZE`

### Objective Function
$$\max Z = 45 \cdot \text{GASOLINE} + 40 \cdot \text{DIESEL} + 28 \cdot \text{NAPHTHA}$$

### Domain Constraints
1. `CRUDE_MAX`: $1.0 \cdot \text{GASOLINE} + 1.0 \cdot \text{DIESEL} + 1.0 \cdot \text{NAPHTHA} \le 100.0$
2. `CRUDE_MIN`: $1.0 \cdot \text{GASOLINE} + 1.0 \cdot \text{DIESEL} + 1.0 \cdot \text{NAPHTHA} \ge 60.0$
3. `PROCESS_CAP`: $1.0 \cdot \text{GASOLINE} + 1.2 \cdot \text{DIESEL} + 0.8 \cdot \text{NAPHTHA} \le 94.0$
4. `DIESEL_MIN`: $1.0 \cdot \text{DIESEL} \ge 20.0$
5. `NAPHTHA_MIN`: $1.0 \cdot \text{NAPHTHA} \ge 10.0$

### Non-negativity & Lower Bounds
- $\text{GASOLINE} \ge 0$
- $\text{DIESEL} \ge 0$
- $\text{NAPHTHA} \ge 0$

---

## 2. Solver Selection & Execution Trace
- **Solver Selected**: `DualRevisedSimplex (CPU)`
- **Routing Rationale**: `GPU_UNAVAILABLE: Native CUDA GPU hardware not detected or GPU disallowed by configuration. Routing to optimal CPU solver (Dual Revised Simplex).`
- **Presolve Reductions**: 2 redundant constraints eliminated (`CRUDE_MAX`, `CRUDE_MIN` bounded by processing and minimum product constraints).
- **Simplex Iterations**: 2 pivots to reach optimality.
- **Solver Status**: `OPTIMAL`

---

## 3. Actual Decision Variable Solution & Objective
- **Gasoline**: `62.00`
- **Diesel**: `20.00`
- **Naphtha**: `10.00`
- **Solver Optimal Objective**: `3870.00`

---

## 4. Independent Objective Recalculation
$$Z_{\text{calc}} = 45(62) + 40(20) + 28(10) = 2790 + 800 + 280 = 3870.00$$

$$\text{Discrepancy} = |Z_{\text{solver}} - Z_{\text{calc}}| = |3870.00 - 3870.00| = 0.000000 \ (\le 1\text{e-}7)$$

---

## 5. Independent Constraint Evaluation Checks

| Constraint | Mathematical Expression | Evaluated Value | Bound Limit | Result |
| :--- | :--- | :--- | :--- | :--- |
| `CRUDE_MAX` | $62 + 20 + 10$ | **92.0** | $\le 100.0$ | **SATISFIED** |
| `CRUDE_MIN` | $62 + 20 + 10$ | **92.0** | $\ge 60.0$ | **SATISFIED** |
| `PROCESS_CAP` | $62 + 1.2(20) + 0.8(10) = 62 + 24 + 8$ | **94.0** | $\le 94.0$ (Active) | **SATISFIED** |
| `DIESEL_MIN` | $20.0$ | **20.0** | $\ge 20.0$ (Active) | **SATISFIED** |
| `NAPHTHA_MIN` | $10.0$ | **10.0** | $\ge 10.0$ (Active) | **SATISFIED** |
| Non-negativity | $G \ge 0, D \ge 0, N \ge 0$ | $G=62, D=20, N=10$ | $\ge 0$ | **SATISFIED** |

---

## 6. C++ SolutionVerifier Results
- **Verifier Status**: `PASS` (`VERIFIED PASS`)
- **Max Constraint Residual $\|A x - b\|_\infty$**: `0.000000e+00` ($\le 1\text{e-}7$)
- **Max Bound Violation**: `0.000000e+00` ($\le 1\text{e-}7$)
- **Max Integrality Violation**: `0.000000e+00` (Continuous LP)
- **Objective Consistency Check**: `Recomputed Objective = 3870.000000`

---

## 7. Measured Execution Runtimes
- **C++ Solver Core Solve Time**: `0.00 ms` (Displayed in UI as `<1 ms`)
- **End-to-End API Bridge & IPC Elapsed Time**: `18.16 ms`

---

## 8. Direct C++ CLI vs. Web/API Cross-Validation

| Attribute | Direct C++ CLI | Web / API Bridge | Match Status |
| :--- | :--- | :--- | :--- |
| **Problem Name** | `refinery_demo` | `refinery_demo` | **EXACT** |
| **Constraints ($M$)** | 5 | 5 | **EXACT** |
| **Variables ($N$)** | 3 | 3 | **EXACT** |
| **Non-Zeros ($\text{NNZ}$)** | 11 | 11 | **EXACT** |
| **Objective Sense** | MAXIMIZE | MAXIMIZE | **EXACT** |
| **Optimal Objective** | `3870.000000` | `3870.000000` | **EXACT** |
| **Gasoline Decision** | `62.0` | `62.0` | **EXACT** |
| **Diesel Decision** | `20.0` | `20.0` | **EXACT** |
| **Naphtha Decision** | `10.0` | `10.0` | **EXACT** |
| **Presolve Reductions** | 2 Rows Eliminated | 2 Rows Eliminated | **EXACT** |
| **Selected Solver** | `DualRevisedSimplex` | `DualRevisedSimplex (CPU)` | **EXACT** |
| **SolutionVerifier** | `PASS` | `VERIFIED PASS` | **EXACT** |

---

## 9. Final Validation Status
**FINAL STATUS: PASS**
