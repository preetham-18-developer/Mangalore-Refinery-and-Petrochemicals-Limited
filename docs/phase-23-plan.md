# Phase 23 — Scalability Testing Implementation Plan

## 1. Goal
Implement an empirical scalability measurement harness evaluating BHARATOPT under dimensions scaling from $10^2$ (100) to $10^6+$ (1,000,000+) variables/constraints, profiling RAM peak memory footprints, sparse storage efficiency, host-device transfer latencies, resource safety safeguards, and machine-readable JSON/CSV telemetry exports.

## 2. Core Components
- **Scalability Telemetry Record**: `ScalabilityResultRecord` struct in `include/bharatopt/scalability_test.hpp` capturing dimensions ($m, n, \text{NNZ}$), sparse density regimes, generation/construction times, baseline RAM, peak RAM, matrix memory estimate, vector memory estimate, GPU VRAM allocation, H2D/kernel/D2H timings, CPU execution time, and explicit execution/failure status.
- **Process Memory Profiler**: Platform-specific memory query (`GetProcessMemoryInfo` via `<psapi.h>` on Windows) to capture working set size and peak working set size.
- **Resource Safeguards**: Resource limits (max RAM limit, max NNZ limit, max time limit) to safely skip ultra-large scales before out-of-memory crashes occur.
- **Sparse Workload Generator**: Deterministic seed-based sparse matrix generator creating controlled sparse workloads without dense matrix allocation.
- **Machine-Readable Exporters**: `ScalabilityReporter::export_csv()` (`phase-23-scalability.csv`) and `ScalabilityReporter::export_json()` (`phase-23-scalability.json`).
- **Dedicated Unit Tests**: `tests/test_scalability.cpp` verifying infrastructure, memory telemetry, resource limits, CSV/JSON output, and GPU status reporting.

## 3. Planned Scalability Workload Dimensions
- $N = 100, M = 50, \text{NNZ} \approx 500$
- $N = 1,000, M = 500, \text{NNZ} \approx 5,000$
- $N = 10,000, M = 5,000, \text{NNZ} \approx 50,000$
- $N = 100,000, M = 50,000, \text{NNZ} \approx 500,000$
- $N = 1,000,000, M = 500,000, \text{NNZ} \approx 5,000,000$ (evaluated under resource safety checks)

## 4. Output Artifacts
- `docs/phase-23-report.md`
- `docs/gates/phase-23-gate.md`
- `phase-23-scalability.csv`
- `phase-23-scalability.json`
