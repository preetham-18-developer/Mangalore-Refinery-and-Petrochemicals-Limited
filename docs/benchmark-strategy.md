# BharatOpt — Benchmarking Strategy & Empirical Performance Methodology

**Document Version:** 1.0.0  
**Date:** September 25, 2026  
**Status:** Approved Testing Standard  

---

## 1. Benchmarking Philosophy & Imperatives

1. **Empirical Measurement Rule:** No algorithm, kernel, or data structure will be claimed as "fast" or "optimal" without reproducible benchmark measurements.
2. **End-to-End Evaluation Rule:** A faster isolated CUDA kernel does **NOT** equal a faster solver. All routing decisions and performance claims must measure **total end-to-end execution time**, including host-device data transfers (H2D and D2H).
3. **No Optimization Without Profiling:** Profiling must identify the exact CPU or GPU bottleneck (memory bandwidth, latency, factorisation, pricing, SpMV) before code modifications are applied.
4. **Reversion Criterion:** If an optimization attempt degrades execution speed or increases memory footprint without a corresponding performance gain, it **MUST BE REVERTED**.

---

## 2. Benchmark Problem Datasets & Matrix Suites

### 2.1 Synthetic Sparse & Dense Matrix Generators
- **Matrix Sizes:** $10 \times 10$, $100 \times 100$, $1,000 \times 1,000$, $10,000 \times 10,000$ (and up to $100,000 \times 100,000$ where memory permits).
- **Non-Zero Densities:** $1\%$, $5\%$, $10\%$, $25\%$, $50\%$.
- **Conditioning:** Well-conditioned diagonally dominant systems, ill-conditioned matrices (condition number $\kappa > 10^6$), and degenerate LP instances.

### 2.2 Standard Benchmark Collections
- **Netlib LP Benchmark Set:** Classic linear programming instances for numerical accuracy and Simplex iteration counting.
- **MIPLIB (Mixed Integer Programming Library):** Benchmark instances for integer relaxation and Branch-and-Bound search tree scaling.

---

## 3. Targeted Micro-Benchmarks & Ablation Studies

Every component will undergo isolated micro-benchmarking:

| Subsystem | Baseline Benchmark | Comparison Candidate | Evaluated Metrics |
| :--- | :--- | :--- | :--- |
| **Matrix Multiply** | Dense Matrix-Vector ($y = Ax$) | Sparse CSR SpMV / Sparse CSC SpMV | Runtime (ms), Memory (MB), NNZ, Throughput (GFLOPS) |
| **Presolve Engine** | Presolve OFF | Presolve ON | Vars Removed, Constraints Removed, NNZ Removed, Total Solve Time |
| **Simplex Solvers** | Educational Simplex | Primal Revised Simplex (LU) / Dual Revised Simplex | Iterations, Pivot Time, Refactorisation Time, Objective Error |
| **PDHG Engine** | CPU Sequential PDHG | GPU CUDA PDHG | H2D Transfer Time, Kernel Execution Time, D2H Time, Convergence Speed |
| **Routing Engine** | Fixed Static Routing | Adaptive `ProblemProfiler` Routing | Routing Accuracy ($\frac{\text{Optimal Path Selected}}{\text{Total Instances}}$), Overhead |

---

## 4. Telemetry Schema & Reporting Format

All benchmark runs will export structured JSON / CSV telemetry records containing the following mandatory fields:

```json
{
  "instance_name": "synth_1000x1000_d05",
  "rows": 1000,
  "cols": 1000,
  "nnz": 50000,
  "density": 0.05,
  "algorithm": "DualRevisedSimplex",
  "execution_target": "CPU",
  "presolve_enabled": true,
  "presolve_vars_removed": 120,
  "presolve_cons_removed": 85,
  "iterations": 1420,
  "factorisations": 35,
  "runtime_ms": 18.45,
  "h2d_transfer_ms": 0.00,
  "kernel_runtime_ms": 0.00,
  "d2h_transfer_ms": 0.00,
  "memory_peak_mb": 12.4,
  "objective_value": 31415.9265,
  "primal_residual": 1.2e-8,
  "dual_residual": 3.4e-8,
  "feasibility_status": "VERIFIED",
  "oracle_reference_objective": 31415.9265,
  "oracle_objective_diff": 0.00e00
}
```

---

## 5. Reference Oracle & Validation Integration

- **External Validation Oracle:** HiGHS (used strictly via CLI or external C API script outside the BharatOpt binary).
- **Oracle Role:** Provides ground-truth optimal objective values, feasibility bounds, and iteration baselines for automated correctness validation.

---
*Maintained as the benchmarking strategy for BharatOpt.*
