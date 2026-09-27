# Phase 22 — Controlled Ablation Studies Implementation Plan

## 1. Goal
Implement a controlled single-variable ablation study harness for BHARATOPT, quantitatively measuring the impact of individual solver components under strictly fixed benchmark conditions without introducing new algorithms or modifying baseline solver behaviour.

## 2. Core Architecture
- **Single-Variable Rule**: Every ablation experiment varies exactly **ONE** factor while keeping all other configuration parameters, model instances, hardware, and tolerances strictly constant.
- **Harness Structure**: `AblationHarness` class in `include/bharatopt/ablation_study.hpp` and `src/ablation_study.cpp`.
- **Isolation & Verification**: Every solved instance in baseline and ablated configurations is independently validated using Phase 20 `SolutionVerifier`.
- **Exporting**: Results exported to machine-readable JSON (`phase-22-ablation.json`) and CSV (`phase-22-ablation.csv`).

## 3. Planned Ablation Experiments (A1 – A7)
1. **A1: Presolve**: `Presolve ON` vs `Presolve OFF`.
2. **A2: Sparse Representation**: `Sparse Representation` (CSC / Sparse LU) vs `Dense / Reference Representation` (`EducationalSimplex` / Dense LU).
3. **A3: Adaptive Routing**: `Adaptive Routing` (`AUTO`) vs Fixed Execution Modes (`FORCE_CPU_REVISED`, `FORCE_CPU_DUAL`, `FORCE_CPU_FIRST_ORDER`, `FORCE_GPU_FIRST_ORDER`).
4. **A4: MILP Warm Starts**: `Warm Start ON` (`WARM_START`) vs `Cold Start ONLY` (`COLD_START`).
5. **A5: Independent Verification Overhead**: `Solve + Verification` vs `Solve ONLY`.
6. **A6: Cost Estimator / Adaptive Decision**: `Adaptive Cost-Based Selection` vs `Fixed Solver Selection`.
7. **A7: MILP Basis Propagation**: `Child Basis Propagation ON` vs `Basis Propagation OFF`.

## 4. Workloads
- Representative synthetic workloads (M1–M10 scaling models).
- Netlib LP benchmark subset (`afiro`, `share2b`).
- MIPLIB MILP benchmark subset (`blend2`, `p0033`).

## 5. Verification & Tests
- Dedicated unit tests in `tests/test_ablation_study.cpp`.
- 100% full regression pass across Debug and Release builds (376 baseline + new Phase 22 tests).
- Zero compiler warnings and zero errors.

## 6. Output Artifacts
- `docs/phase-22-report.md`
- `docs/gates/phase-22-gate.md`
