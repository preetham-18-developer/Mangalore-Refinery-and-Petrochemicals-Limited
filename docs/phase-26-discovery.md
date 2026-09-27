# Phase 26 Discovery — SIH Final CLI & Dashboard

## Original Roadmap Definition
According to the master plan ([docs/master-plan.md](file:///c:/Users/PREETHAM/OneDrive/%E3%83%89%E3%82%AD%E3%83%A5%E3%83%A1%E3%83%B3%E3%83%88/Desktop/Algorithm-Project/docs/master-plan.md) L119) and problem requirements ([docs/requirements.md](file:///c:/Users/PREETHAM/OneDrive/%E3%83%89%E3%82%AD%E3%83%A5%E3%83%A1%E3%83%B3%E3%83%88/Desktop/Algorithm-Project/docs/requirements.md) REQ-10):
- **Phase Title:** Phase 26 — SIH Final CLI & Dashboard
- **Phase Index:** 26 of 27
- **Defined Location:** `docs/gates/phase-26-gate.md`

## Objective
[ORIGINAL ROADMAP REQUIREMENT]
Production CLI demonstration pipeline displaying presolve stats, adaptive routing decisions, CPU/GPU solve metrics, benchmark summaries, and independent solution verification status for SIH 2026 PS 26119 demonstration.

## Original Deliverables
[ORIGINAL ROADMAP REQUIREMENT]
1. **Production CLI Executable (`bharatopt_cli`):**
   - Full command-line argument parser (`--file <path>`, `--solver <auto|revised|dual|pdhg|bnb>`, `--presolve <on|off>`, `--benchmark-summary`, `--verify`).
   - Support for MPS model loading, LP model instantiation, presolve reduction execution, solver execution, and verification.
2. **Terminal Dashboard Renderer:**
   - Formatted console display displaying problem statistics (rows, cols, NNZ, density), presolve reduction stats, adaptive router selection, solve progress, solution summary, and `SolutionVerifier` status.
3. **Telemetry & Benchmark Display:**
   - Consumes Phase 25 telemetry (`phase-25-final-benchmark.json` and `.csv`) to display comparative benchmark summary tables directly in terminal output.
4. **Dedicated Test Suite:**
   - `tests/test_cli_dashboard.cpp`
5. **Documentation Artifacts:**
   - `docs/phase-26-report.md`
   - `docs/gates/phase-26-gate.md`

## Original Non-Goals
[ORIGINAL ROADMAP REQUIREMENT]
- Do NOT introduce new solver core algorithms, CUDA kernels, or QP solvers.
- Do NOT build external web GUIs or React/Node.js web servers unless specified in C++ terminal dashboard.
- Do NOT modify solver mathematics or numerical tolerances.
- Do NOT start Phase 27 or post-roadmap gap-closure work.

## Dependencies
[ORIGINAL ROADMAP REQUIREMENT]
- **Phase 20:** Independent solution verifier (`SolutionVerifier`).
- **Phase 21:** MPS model parser (`MpsParser`).
- **Phase 25:** Final benchmark telemetry exports (`phase-25-final-benchmark.csv` and `phase-25-final-benchmark.json`).

## Phase 25 Integration
[ORIGINAL ROADMAP REQUIREMENT]
Phase 26 consumes Phase 25 outputs:
- `phase-25-final-benchmark.json`
- `phase-25-final-benchmark.csv`

The CLI dashboard parses these machine-readable records to render standardized benchmark summary tables, feasibility verification rates, and win/loss breakdowns for SIH demonstration.

## Current Repository Status
[CURRENT REPOSITORY STATUS]
- **Verified Regression Baseline:** 435 / 435 PASS across Release and Debug builds with 0 compiler warnings and 0 errors.
- **Phases 0–25 State:** COMPLETE and VERIFIED.
- **Phase 26 Implementation:** NOT IMPLEMENTED (Discovery phase only).

| Component | Existing Implementation | File Reference | Status |
|---|---|---|---|
| Basic CLI Binary | Version banner and init message | `src/main.cpp` | PARTIAL |
| CLI Argument Parser | Full flag parsing (`--file`, `--solver`, `--verify`) | N/A | NOT IMPLEMENTED |
| Terminal Dashboard | Console layout for presolve, router, & verifier | N/A | NOT IMPLEMENTED |
| Benchmark Telemetry Viewer | Parser for `phase-25-final-benchmark.json` | N/A | NOT IMPLEMENTED |
| Dedicated Test Suite | `tests/test_cli_dashboard.cpp` | N/A | NOT IMPLEMENTED |
| Documentation | `docs/phase-26-report.md` / `docs/gates/phase-26-gate.md` | N/A | NOT IMPLEMENTED |

## Missing Components
[CURRENT REPOSITORY STATUS]
1. `include/bharatopt/cli_dashboard.hpp` — Header for CLI argument parser and terminal dashboard renderer.
2. `src/cli_dashboard.cpp` — Implementation of argument handling, console dashboard rendering, and telemetry viewing.
3. `src/main.cpp` — Updated CLI main function delegating to `cli_dashboard`.
4. `tests/test_cli_dashboard.cpp` — Dedicated Phase 26 unit test suite.
5. `docs/phase-26-report.md` — Final CLI demonstration report.
6. `docs/gates/phase-26-gate.md` — Phase 26 gate document.

## Environment Requirements
[CURRENT REPOSITORY STATUS]
- **Native CUDA:** `NOT_AVAILABLE` (CPU fallback `CPU_FALLBACK` displayed in dashboard).
- **External HiGHS Oracle:** `NOT_AVAILABLE` (`SolutionVerifier` status displayed).
- **Toolchain:** Standard C++ STL, MinGW GCC / LLVM Clang.
- **Python / Node.js / React / Databases:** `NOT_REQUIRED`.

## Verification Requirements
[ORIGINAL ROADMAP REQUIREMENT]
- Command-line flag parsing correctly validates MPS paths, solver options, and output switches.
- Terminal dashboard displays presolve reduction stats, router choices, objective values, and verifier status cleanly.
- 100% regression suite pass (435 baseline + new Phase 26 dedicated tests).
- Zero compiler warnings and zero errors in Debug and Release builds.

## Original Gate Criteria
[ORIGINAL ROADMAP REQUIREMENT]
- **STATUS:** IMPLEMENTED
- **VERIFICATION:** VERIFIED
- **DEDICATED TESTS:** PASS
- **FULL REGRESSION:** PASS (435 + Phase 26 tests)
- **DEBUG BUILD:** PASS
- **RELEASE BUILD:** PASS
- **CLI ARGUMENT PARSING:** PASS
- **TERMINAL DASHBOARD:** PASS
- **TELEMETRY VIEWING:** PASS
- **WARNINGS:** 0
- **ERRORS:** 0
- **GATE:** PASS

## Recommended Implementation Order
[RECOMMENDATION]
1. **Define CLI Interface:** Create `include/bharatopt/cli_dashboard.hpp` specifying `CLIOptions` and `TerminalDashboard`.
2. **Implement CLI & Dashboard:** Implement argument parsing, formatted console renderer, and telemetry viewer in `src/cli_dashboard.cpp`.
3. **Update Main Binary:** Wire `src/main.cpp` to execute `CLIDashboard::run(argc, argv)`.
4. **Create Dedicated Tests:** Write `tests/test_cli_dashboard.cpp` testing flag parsing, dashboard text formatting, and telemetry ingestion.
5. **Register Target:** Add `src/cli_dashboard.cpp` and `tests/test_cli_dashboard.cpp` to `CMakeLists.txt` and `tests/CMakeLists.txt`.
6. **Execute Regression:** Run unit tests across Debug and Release build configurations.
7. **Generate Documentation:** Create `docs/phase-26-report.md` and `docs/gates/phase-26-gate.md`.

## Conclusion
Phase 26 discovery is complete. The exact scope, dependencies, Phase 25 integration, and implementation plan have been established. Implementation will remain paused until explicitly requested.
