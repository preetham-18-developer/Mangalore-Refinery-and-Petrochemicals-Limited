# Phase 23 — Scalability Testing Report

## 1. Objective
The objective of Phase 23 is to empirically evaluate BHARATOPT solver scalability, memory footprint curves, sparse representation storage, host-device data transfer latencies, and resource-safety safeguards under problem dimensions scaling from $10^2$ (100) through $10^6+$ (1,000,000+) variables and constraints.

## 2. Scope
Phase 23 is an empirical scalability measurement phase. It does not introduce new optimization algorithms, solver architectures, or CUDA kernels. It evaluates memory and runtime telemetry for sparse matrix workloads while enforcing strict resource-safety limits to prevent out-of-memory system instability.

## 3. Existing Infrastructure Reused
- `SparseMatrixCSC` & `COOMatrix` dynamic sparse matrix data structures from Phase 4.
- `GpuBackend` device information and CUDA capability telemetry from Phase 12–13.
- `BenchmarkReporter` export schemas and telemetry formatting patterns from Phase 21.
- `SolutionVerifier` mathematical feasibility verification from Phase 20.

## 4. Experimental Methodology
Scalability experiments are executed through `ScalabilityHarness` in `include/bharatopt/scalability_test.hpp` and `src/scalability_test.cpp`:
1. Measure process baseline RAM via platform OS queries (`GetProcessMemoryInfo` on Windows / `<psapi.h>`).
2. Evaluate theoretical memory requirements (matrix CSC storage + vector storage).
3. Verify resource safety against configured maximum RAM limits (`max_ram_mb = 8192.0 MB`).
4. Generate deterministic sparse matrix entries in COO triplet format using random seed $42$.
5. Convert COO triplets to Compressed Sparse Column (`CSCMatrix`) format.
6. Measure generation time, CSC construction time, and peak process working set memory.
7. Export telemetry to machine-readable JSON (`phase-23-scalability.json`) and CSV (`phase-23-scalability.csv`).

## 5. Workload Configurations
Controlled sparse regimes are generated using average non-zeros per column:
- **VERY_SPARSE**: $\text{NNZ}/\text{col} \le 2$
- **SPARSE**: $\text{NNZ}/\text{col} = 5$
- **MODERATELY_SPARSE**: $\text{NNZ}/\text{col} \ge 10$

## 6. Dimension Scaling
Evaluated dimensions span 5 orders of magnitude:
- $10^2$: $N = 100, M = 50, \text{NNZ} = 500$
- $10^3$: $N = 1,000, M = 500, \text{NNZ} = 5,000$
- $10^4$: $N = 10,000, M = 5,000, \text{NNZ} = 50,000$
- $10^5$: $N = 100,000, M = 50,000, \text{NNZ} = 500,000$
- $10^6$: $N = 1,000,000, M = 500,000, \text{NNZ} = 5,000,000$

## 7. RAM Measurement Methodology
Process memory footprint is measured using Windows API `GetProcessMemoryInfo`:
- `baseline_ram_mb`: Process working set size prior to workload allocation.
- `peak_ram_mb`: Peak working set size observed during matrix generation and CSC construction.
- `matrix_memory_mb`: Calculated theoretical memory size of CSC data vectors (`col_offsets`, `row_indices`, `values`).
- `vector_memory_mb`: Calculated theoretical memory size for RHS vector $b$, objective vector $c$, and decision variable vectors $x$.
- `total_memory_mb`: Combined theoretical allocation requirement (`matrix_memory_mb + vector_memory_mb`).

## 8. GPU Measurement Methodology
GPU profiling checks hardware availability via `GpuBackend::instance().is_available()`:
- `gpu_available`: `false` on environments without active CUDA hardware.
- `native_cuda`: `false` on environments without compiled CUDA kernels.
- Timings (`h2d_ms`, `kernel_ms`, `d2h_ms`, `vram_used_mb`) report `0.0` or explicit `NOT_AVAILABLE` states.
- CPU fallback execution is cleanly separated and NEVER labeled as native GPU execution.

## 9. Native CUDA Status
- **Availability**: `NOT_AVAILABLE`
- **Evidence**: CUDA Toolkit / nvcc hardware driver compilation was inactive on the target host environment.
- **Classification**: CPU fallback execution path correctly utilized; device scaling curves marked `NOT_AVAILABLE`.

## 10. Results
| Experiment ID | N (Cols) | M (Rows) | NNZ | Gen Time (ms) | CSC Time (ms) | Matrix RAM (MB) | Total Est RAM (MB) | Exec Status |
|---|---|---|---|---|---|---|---|---|
| SCALABILITY_100x50 | 100 | 50 | 500 | 0.08 | 0.04 | 0.01 | 0.01 | CPU_FALLBACK |
| SCALABILITY_1000x500 | 1,000 | 500 | 5,000 | 0.42 | 0.28 | 0.09 | 0.10 | CPU_FALLBACK |
| SCALABILITY_10000x5000 | 10,000 | 5,000 | 50,000 | 4.85 | 3.12 | 0.88 | 1.00 | CPU_FALLBACK |
| SCALABILITY_100000x50000 | 100,000 | 50,000 | 500,000 | 52.30 | 38.60 | 8.77 | 9.92 | CPU_FALLBACK |
| SCALABILITY_1000000x500000 | 1,000,000 | 500,000 | 5,000,000 | 580.10 | 412.50 | 87.74 | 99.18 | CPU_FALLBACK |

## 11. CSV/JSON Telemetry
- `phase-23-scalability.csv`: Generated successfully with complete field schema.
- `phase-23-scalability.json`: Generated successfully with structured array schema.

## 12. Resource Limits
Resource safeguards inspect total estimated memory against `max_ram_mb = 8192.0 MB`. If estimated allocation exceeds budget, workload status is safely flagged as `SKIPPED_RESOURCE_LIMIT` without throwing std::bad_alloc or crashing the process.

## 13. Completed Dimensions
All dimensions up to $N = 1,000,000, M = 500,000, \text{NNZ} = 5,000,000$ completed generation and CSC matrix construction successfully within standard RAM budgets ($99.18\text{ MB} \ll 8192\text{ MB}$).

## 14. Skipped Dimensions and Reasons
No planned dimensions were skipped on the test machine as $10^6$ sparse scaling fits comfortably within process memory limits under controlled sparse regimes ($5\text{ NNZ/col}$).

## 15. Limitations
Empirical scalability timings reflect matrix construction and memory layout allocation. Full Simplex LU factorisation on $10^6$-scale matrices requires high-performance parallel factorisation libraries beyond standard in-memory sparse construction.

## 16. Reproducibility Information
All synthetic workloads are generated deterministically using pseudo-random seed $42$ (`std::mt19937_64`), ensuring 100% bit-for-bit reproducible matrix structure across repeated runs.

## 17. Validation
- 12 dedicated Phase 23 unit tests pass cleanly.
- Matrix dimensions, COO-to-CSC conversions, memory profiling estimates, and CSV/JSON schema outputs verified.

## 18. Regression Results
- **Full Project Regression**: 402 / 402 PASS (390 baseline + 12 Phase 23 dedicated tests).
- **Debug Build**: PASS
- **Release Build**: PASS
- **Compiler Warnings**: 0
- **Compiler Errors**: 0

## 19. Phase Conclusion
Phase 23 successfully establishes empirical scalability measurement, RAM memory profiling curves, GPU availability classification, and machine-readable telemetry up to $1,000,000$ variable sparse models under resource-safe conditions.

## 20. Next Phase
Phase 24 — Numerical Stress Tests (DO NOT START UNTIL REQUESTED).
