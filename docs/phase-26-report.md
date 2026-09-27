# Phase 26 — SIH Final CLI & Dashboard Report

## Objective

The objective of Phase 26 is to create a production-style C++ command-line demonstration pipeline and terminal dashboard for **BHARATOPT** (SIH 2026 PS 26119 for MRPL). The dashboard acts as a presentation and integration layer over the existing validated solver infrastructure, presenting model statistics, validation results, presolve statistics, adaptive routing decisions, solver execution metrics, objective values, runtime, independent mathematical solution verification, Phase 25 comparative benchmark telemetry, and documented environmental limitations.

---

## Implemented Components

The following files were created/updated to implement Phase 26:

1. **`include/bharatopt/cli_dashboard.hpp`**: Header declaring CLI options structure (`CLIOptions`), CLI argument parser (`CLIParser`), formatting dashboard renderer (`TerminalDashboard`), and main application entry point (`CLIDashboardApp`).
2. **`src/cli_dashboard.cpp`**: Core implementation of argument parsing (`--file`, `--solver`, `--presolve`, `--verify`, `--benchmark-summary`, `--help`), formatted terminal output, solver pipeline execution, postsolve recovery, verifier invocation, and telemetry summary viewing.
3. **`src/main.cpp`**: Updated `main()` entry point delegating directly to `bharatopt::CLIDashboardApp::run(argc, argv)`.
4. **`CMakeLists.txt`**: Added `src/cli_dashboard.cpp` to the `bharatopt_core` static library target.
5. **`tests/CMakeLists.txt`**: Added `tests/test_cli_dashboard.cpp` to the `bharatopt_tests` test executable target.
6. **`tests/test_cli_dashboard.cpp`**: Created 30 dedicated unit tests covering CLI options, argument parser, terminal dashboard formatting, telemetry ingestion, verifier display, malformed input safety, and documented environment limitations.

---

## CLI Interface

The executable `bharatopt_cli` supports the following syntax:

```bash
bharatopt_cli --file <path.mps> [options]
bharatopt_cli --benchmark-summary
```

### Options

| Flag | Short | Description |
|---|---|---|
| `--file <path>` | `-f` | Input MPS format optimization model file |
| `--solver <choice>` | `-s` | Solver choice: `auto` (default), `revised`, `dual`, `pdhg`, `bnb` |
| `--presolve <on\|off>` | `-p` | Enable/disable presolve reductions (default: `on`) |
| `--verify` | `-v` | Invoke standalone mathematical `SolutionVerifier` against original model |
| `--benchmark-summary` | `-b` | Display Phase 25 comparative benchmark summary table |
| `--help` | `-h` | Display help message and syntax |

---

## Dashboard Sections

The `TerminalDashboard` formats and outputs the following sections:

1. **HEADER**: Version, SIH 2026 PS 26119 details, target organization (MRPL).
2. **MODEL**: Filename, problem name, problem type (LP vs MILP), rows ($M$), columns ($N$), non-zeros ($\text{NNZ}$), sparsity density, variable counts (continuous/integer/binary), objective sense.
3. **VALIDATION**: Model structural validation status (PASS/FAIL), warning/error counts and detailed diagnostic messages.
4. **PRESOLVE**: Presolve policy (ENABLED/DISABLED), original/reduced row and column counts, variables/constraints eliminated, presolve runtime.
5. **ADAPTIVE ROUTER**: Routing mode (AUTO vs FORCED), selected solver, target device (`CPU`/`GPU`/`CPU_FALLBACK`), routing rationale, predicted costs.
6. **SOLVE**: Solver status (`OPTIMAL`, `INFEASIBLE`, `UNBOUNDED`, `MAX_ITERATIONS`), optimal objective value, simplex iterations, B&B tree nodes, solve runtime, end-to-end total runtime.
7. **VERIFICATION**: Standalone `SolutionVerifier` status (`PASS`/`FAIL`), constraint feasibility violation residual, bound violation, integrality violation, recomputed original objective.
8. **ENVIRONMENT**: Honest reporting of native CUDA (`NOT_AVAILABLE`) and HiGHS oracle status (`NOT_AVAILABLE`).
9. **BENCHMARK SUMMARY**: Phase 25 comparative telemetry summary table and 1M scalability classification (`CONSTRUCTED`).

---

## Solver Integration

The CLI options map directly to existing validated BHARATOPT solvers:

- `--solver auto`: Invokes `ExecutionRouter` for automatic workload feature profiling and solver selection.
- `--solver revised`: Invokes `RevisedSimplex` with sparse LU factorization.
- `--solver dual`: Invokes `DualRevisedSimplex`.
- `--solver pdhg`: Invokes `FirstOrderLPSolver` (CPU PDHG implementation).
- `--solver bnb`: Invokes `BranchAndBoundEngine` for MILP models.

---

## Environmental Limitations & Scalability Classification

The CLI dashboard honestly reports all documented system environment limitations:

1. **Native CUDA**: `NOT_AVAILABLE` (CPU reference solver active; CUDA compiler driver inactive in current platform environment).
2. **HiGHS Oracle**: `NOT_AVAILABLE` (Mathematical correctness verified via standalone `SolutionVerifier`).
3. **1M $\times$ 500k / 5M NNZ Scalability**: `CONSTRUCTED` (Classified as sparse matrix memory construction benchmark of 99.18 MB, not a 1-million-variable LP optimization solve).

---

## Test Results

### Dedicated Phase 26 Unit Tests
- **Location**: `tests/test_cli_dashboard.cpp`
- **Result**: **30 / 30 PASS**

### Full System Regression Suite
- **Location**: `tests/`
- **Baseline (Phases 0–25)**: 435 tests
- **Phase 26 Additions**: 30 tests
- **Total Suite Result**: **465 / 465 PASS**

### Compiler Warnings & Errors
- **Debug Build Warnings**: 0
- **Debug Build Errors**: 0
- **Release Build Warnings**: 0
- **Release Build Errors**: 0

---

## Demonstration Outputs

### 1. Benchmark Telemetry Summary (`bharatopt_cli --benchmark-summary`)

```
========================================================
 BHARATOPT OPTIMISATION SOLVER ENGINE
 BharatOpt v0.1.0 (C++17, Double Precision)
 SIH 2026 PS 26119 | Target: MRPL
========================================================

========================================================
 PHASE 25 FINAL BENCHMARK SUMMARY
========================================================

=========================================================================================
                                BHARATOPT BENCHMARK MATRIX                              
=========================================================================================
Instance ID         Solver              Dim (m x n) NNZ       Time (ms)   Iters     Objective   Verify    
-----------------------------------------------------------------------------------------
SYNTH_LP_42         RevisedSimplex-CPU_ 50x100      500       1.25        15        142.50      PASS      
SYNTH_MILP_42       BranchAndBound-CPU  25x50       250       8.40        0         85.00       PASS      
afiro               RevisedSimplex-CPU_ 27x32       88        0.45        12        -464.75     PASS      
p0033               BranchAndBound-CPU  16x33       98        3.10        0         3089.00     PASS      
-----------------------------------------------------------------------------------------
=========================================================================================

[ ENVIRONMENT & LIMITATIONS ]
  Native CUDA Status  : NOT_AVAILABLE
  HiGHS Oracle Status : NOT_AVAILABLE
  1M Scalability      : CONSTRUCTED (Sparse Matrix Memory Benchmark - 99.18 MB)
========================================================
```

### 2. Real MPS LP Solve (`bharatopt_cli --file benchmarks/netlib/afiro.mps --solver auto --presolve on --verify`)

```
========================================================
 BHARATOPT OPTIMISATION SOLVER ENGINE
 BharatOpt v0.1.0 (C++17, Double Precision)
 SIH 2026 PS 26119 | Target: MRPL
========================================================

[ MODEL ]
  File Name           : benchmarks/netlib/afiro.mps
  Problem Name        : AFIRO
  Problem Type        : LP (Linear Program)
  Constraints (M)     : 27
  Variables (N)       : 32
  Non-Zeros (NNZ)     : 88
  Sparsity Density    : 10.19%
  Variable Types      : Continuous=32, Integer=0, Binary=0
  Objective Sense     : MINIMIZE

[ VALIDATION ]
  Validation Status   : PASS

[ PRESOLVE ]
  Presolve Policy     : ENABLED
  Original Rows       : 27
  Reduced Rows        : 27 (Removed: 0)
  Original Columns    : 32
  Reduced Columns     : 32 (Removed: 0)
  Presolve Time       : 0.12 ms

[ ADAPTIVE ROUTER ]
  Routing Mode        : AUTO (Adaptive Profiler)
  Selected Solver     : RevisedSimplex
  Execution Target    : CPU
  Routing Rationale   : Small sparse LP model suitable for exact Revised Simplex.
  Predicted CPU Cost  : 0.50 ms
  Predicted GPU Cost  : 15.00 ms

[ SOLVE ]
  Solver Status       : OPTIMAL
  Optimal Objective   : -464.753143
  Simplex Iterations  : 12
  B&B Tree Nodes      : 0
  Solve Time          : 0.45 ms
  Total End-to-End    : 0.65 ms

[ VERIFICATION ]
  SolutionVerifier    : PASS
  Status Message      : Candidate solution satisfies all constraints and bounds within feasibility tolerance.
  Max Constraint Viol : 0.000000e+00
  Max Bound Viol      : 0.000000e+00
  Max Integrality Viol: 0.000000e+00
  Recomputed Objective: -464.753143

[ ENVIRONMENT ]
  Native CUDA Status  : NOT_AVAILABLE (Compiler driver inactive; CPU reference active)
  HiGHS Oracle Status : NOT_AVAILABLE (Validation uses independent SolutionVerifier)
```

---

## Verification Evidence

- Dedicated CLI unit tests verify argument parser edge cases, invalid flags, missing files, solver routing options, presolve control, verifier displays, and telemetry ingestion.
- MPS parser and solver pipeline integration verified on real Netlib (`afiro.mps`, `share2b.mps`) and MIPLIB (`p0033.mps`, `blend2.mps`) instances.
- Zero existing solver core mathematics or numerical tolerances modified.
- All 465 unit and integration tests PASS across Debug and Release builds.

---

## Conclusion

Phase 26 is completely implemented, verified, and closed. BHARATOPT now features a production-style C++ CLI and terminal dashboard ready for SIH 2026 PS 26119 demonstration.
