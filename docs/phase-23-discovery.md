# Phase 23 Discovery & Scope Confirmation Report

## 1. Phase 23 Title
**Phase 23 — Scalability Testing**

## 2. Original Objective
To evaluate BHARATOPT solver performance, scaling behaviour, memory consumption, and GPU utilization curves under problem sizes scaling from 100 to 1,000,000+ (1M+) variables and constraints.

## 3. Original Deliverables
1. **Large-Scale Dimension & Density Scaling Suite**: Benchmark harness expanding instance generation and evaluation from $10^2$ (100) to $10^6$ (1M+) variables/constraints across varying non-zero densities ($1\% - 50\%$).
2. **Memory Footprint & Allocation Curves**: Tracking peak RAM allocation (MB/GB) as a function of model size ($N, M, \text{NNZ}$) for matrix construction, LU factorisation, and simplex tableau operations.
3. **GPU Utilization & Transfer Scaling**: Profiling CUDA VRAM allocation, host-to-device (H2D) transfer scaling, device-to-host (D2H) transfer scaling, and CUDA compute kernel execution scaling across problem dimensions.
4. **Machine-Readable Telemetry & Gate Artifacts**:
   - `phase-23-scalability.csv`
   - `phase-23-scalability.json`
   - `docs/phase-23-report.md`
   - `docs/gates/phase-23-gate.md`

## 4. Current Implementation Status
| Deliverable | Status | Evidence / Reference Files |
|---|---|---|
| Benchmark Generator / Scaling Harness | **PARTIAL** | `BenchmarkGenerator` in [include/bharatopt/benchmark_framework.hpp](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/include/bharatopt/benchmark_framework.hpp) supports scaling up to 10,000 dimensions. Needs extension to 1M+ dimensions. |
| Memory Footprint Tracking | **NOT IMPLEMENTED** | Peak memory allocation curves are not currently tracked in `BenchmarkResultRecord`. |
| GPU Utilization Scaling Curves | **PARTIAL** | H2D/kernel/D2H timer fields exist in `BenchmarkResultRecord`, but VRAM utilization curves across scaling dimensions ($10^2 - 10^6$) are not collected. |
| Phase 23 Telemetry & Gate Reports | **NOT IMPLEMENTED** | `phase-23-scalability.json`, `phase-23-scalability.csv`, `docs/phase-23-report.md`, `docs/gates/phase-23-gate.md` do not exist. |

## 5. Dependencies
- **Phase 0–22 Baseline**: Verified solver library (`bharatopt_core`), presolve engine, sparse matrix engine, execution router, solution verifier, MPS parser, and ablation study framework.
- **System Memory Constraints**: Memory profiling must safely detect system RAM and VRAM limits to handle ultra-large instances without out-of-memory crashes.

## 6. Remaining Work for Phase 23
1. Implement memory utilization telemetry (tracking peak heap RAM allocation per instance).
2. Extend synthetic scaling benchmarks to evaluate dimensions $10^2, 10^3, 10^4, 10^5, 10^6$.
3. Measure H2D, kernel, D2H, and VRAM utilization scaling across problem sizes (reporting `NOT_AVAILABLE` when CUDA hardware compilation is absent).
4. Implement `Phase23_ScalabilityRunner` and export results to `phase-23-scalability.csv` and `phase-23-scalability.json`.
5. Create dedicated Phase 23 tests in `tests/test_scalability.cpp`.
6. Generate `docs/phase-23-report.md` and `docs/gates/phase-23-gate.md`.

## 7. Recommended Implementation Boundary
Phase 23 must focus exclusively on empirical scalability measurement, memory utilization curves, and GPU throughput curves. No new optimization algorithms, new GPU kernels, or cutting planes will be implemented.

## 8. Relevant Repository Files
- [docs/master-plan.md](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/docs/master-plan.md#L116)
- [docs/benchmark-strategy.md](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/docs/benchmark-strategy.md#L20-L28)
- [include/bharatopt/benchmark_framework.hpp](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/include/bharatopt/benchmark_framework.hpp)
- [src/benchmark_framework.cpp](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/src/benchmark_framework.cpp)
