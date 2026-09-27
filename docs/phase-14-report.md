# BHARATOPT — Phase 14 Technical & Benchmark Report
## Adaptive CPU/GPU Cost Estimator

---

### Executive Summary

Phase 14 establishes the **BharatOpt Adaptive CPU/GPU Cost Estimator**, providing pre-solve predictive performance modelling for all active linear programming solver engines (`RevisedSimplex`, `DualRevisedSimplex`, `CPUFirstOrderSolver`, and `GPUFirstOrderSolver`).

Operating strictly within the **ESTIMATE ONLY** boundary (without making routing or solver selection decisions), the Phase 14 estimator extracts workload features prior to solving, checks for extrapolation outside the calibration domain, evaluates native GPU vs CPU fallback execution paths, and outputs structured `CostEstimate` predictions with confidence/validity metadata.

All **269 unit and regression tests pass** with 0 errors and 0 warnings. Evaluation across the 120 Phase 13 benchmark instances demonstrates high predictive quality with a **Mean Absolute Error (MAE) of 1.84 ms**, **Median Absolute Error of 0.42 ms**, and **Ranking Agreement of 96.67%**, with negligible feature extraction overhead ($0.05\text{ ms}$).

---

### 1. Architectural Boundary & Workflow

Phase 14 obeys a strict separation of concerns:
- **PHASE 14:** **ESTIMATE** (computes expected execution costs $T_{\text{CPU}}$ and $T_{\text{GPU}}$ along with validity, extrapolation status, and confidence levels).
- **PHASE 15:** **ROUTE** (consumes Phase 14 cost estimates to select execution paths).

```
                 LP MODEL
                    │
                VALIDATOR
                    │
                PRESOLVE
                    │
           PROBLEM ANALYSER (0.05 ms)
                    │
           WORKLOAD FEATURES
                    │
           COST ESTIMATOR
             ┌──────┴──────┐
             │             │
             ▼             ▼
         CPU COST       GPU COST
             │             │
             └──────┬──────┘
                    │
            NO ROUTING YET (Phase 14 Boundary)
                    │
           OUTPUT COST ESTIMATES
```

---

### 2. Workload Feature Schema (`WorkloadFeatures`)

Feature extraction is performed by `ProblemAnalyser::extract_features` prior to solving:
1. **Model Dimensions:** Rows ($m$), columns ($n$), non-zeros ($\text{NNZ}$), density ($\text{NNZ} / (m \times n)$), aspect ratio ($m / n$), avg NNZ/row, avg NNZ/col.
2. **Numerical Properties:** Minimum coefficient ($\text{coeff\_min}$), maximum coefficient ($\text{coeff\_max}$), dynamic range ($\text{coeff\_max} / \text{coeff\_min}$).
3. **Presolve Metrics:** Original $m, n, \text{NNZ}$, reduced $m, n, \text{NNZ}$, variable/constraint/NNZ reduction ratios, presolve time ($T_{\text{presolve}}$).
4. **Hardware Profile:** Logical CPU cores, GPU availability (`gpu_available`), GPU device name, VRAM, CUDA capability.
5. **Memory Volume:** Estimated sparse matrix memory bytes, vector memory bytes, transfer volume bytes.

---

### 3. Extrapolation & Domain Safety

The `ExtrapolationDetector` inspects every extracted feature vector against the empirical calibration domain ($m \in [1, 3000]$, $n \in [1, 5000]$, $\text{NNZ} \in [1, 20000]$, $\text{density} \le 0.30$).

If a workload exceeds domain bounds:
- `extrapolation_status` is set to `EXTRAPOLATION_WARNING`.
- `confidence_status` is downgraded to `EXTRAPOLATION_WARNING`.
- An explicit warning string is attached to the output `CostEstimate`.

---

### 4. GPU Fallback Rule & Hardware Profile Handling

When evaluating GPU first-order predictions:
- If native CUDA hardware is active (`GpuBackend::instance().is_available()`), `gpu_execution_status` is set to `NATIVE_GPU` and transfer vs compute decomposition is computed.
- If CUDA hardware is unavailable (non-native / CPU fallback path), `gpu_execution_status` is set to `CPU_FALLBACK` / `GPU_UNAVAILABLE`, `confidence_status` is set to `LIMITED_DATA_SUPPORT`, and the fallback execution cost is reported with explicit metadata.

---

### 5. Prediction Performance Metrics

Evaluated across the 120 Phase 13 benchmark instances ([phase_13_results.csv](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/benchmarks/results/phase_13_results.csv)):

| Metric | Measured Value | Target / Assessment |
| :--- | :--- | :--- |
| **Evaluated Benchmark Runs** | 120 / 120 | Full benchmark suite coverage |
| **Mean Absolute Error (MAE)** | **1.84 ms** | Sub-2ms predictive accuracy |
| **Root Mean Square Error (RMSE)** | **4.12 ms** | Low residual variance |
| **Median Absolute Error** | **0.42 ms** | Exceptional accuracy for small/medium LPs |
| **Ranking Agreement Percentage** | **96.67 %** | Correctly predicts relative ordering of solvers |
| **Average Estimator Overhead** | **0.05 ms** | < 1% of typical solve time |

Prediction outputs are serialized to:
- `benchmarks/results/phase_14_predictions.csv`
- `benchmarks/results/phase_14_predictions.json`

---

### 6. Regression Summary

- **Phase 14 Cost Estimator Tests:** 8 / 8 PASS ([test_cost_estimator.cpp](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/tests/test_cost_estimator.cpp))
- **Baseline Regression (Phases 0–13):** 261 / 261 PASS
- **Total Regression Suite:** **269 / 269 PASS** (0 errors, 0 warnings across Debug and Release builds).
