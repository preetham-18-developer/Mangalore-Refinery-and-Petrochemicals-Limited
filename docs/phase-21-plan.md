# Phase 21 Plan — Benchmark Framework

## 1. Overview
Phase 21 establishes an automated, standardized **Benchmark Framework** for BharatOpt. The framework integrates:
1. Synthetic benchmark workload matrices (M1–M10).
2. Standardized MPS format file parsing (`MpsParser`) for LP and MILP instances.
3. Netlib LP benchmark suite integration (`benchmarks/netlib/`).
4. MIPLIB MILP benchmark suite integration (`benchmarks/miplib/`).
5. Isolated external HiGHS oracle reference comparison adapter (`HighsOracleAdapter`).
6. Integration with Phase 20's `SolutionVerifier` for automated solution verification across all benchmark runs.
7. Machine-readable result exporters (CSV and JSON).

---

## 2. Critical Architectural Rules & Non-Goals
- **Oracle Isolation:** HiGHS is strictly an **EXTERNAL BENCHMARK ORACLE ONLY**. It must NOT be linked into BharatOpt solver libraries, used by solver algorithms, referenced in presolve, invoked by the adaptive router, or used by the independent verifier.
- **Independent Solver:** BharatOpt solvers remain 100% self-contained and indigenous.
- **No Algorithm Modifications:** No new optimization algorithms, simplex variants, GPU solvers, or cutting planes will be implemented in Phase 21.

---

## 3. MPS Parser Architecture

### `MpsParser` Header: `include/bharatopt/mps_parser.hpp`
### `MpsParser` Implementation: `src/mps_parser.cpp`

#### Supported MPS Sections:
- **`NAME`**: Extracts problem identifier.
- **`OBJSENSE`**: Parses `MIN` / `MAX` / `MINIMIZE` / `MAXIMIZE` objective sense.
- **`ROWS`**: Parses row types:
  - `N`: Objective row or free row.
  - `E`: Equality constraint ($Ax = b$).
  - `L`: Less-than-or-equal constraint ($Ax \le b$).
  - `G`: Greater-than-or-equal constraint ($Ax \ge b$).
- **`COLUMNS`**: Parses matrix coefficients and integer markers (`'MARKER'`, `'INTORG'`, `'INTEND'`).
- **`RHS`**: Parses right-hand side vector values.
- **`RANGES`**: Parses ranged constraints ($l \le Ax \le u$).
- **`BOUNDS`**: Parses bound types:
  - `LO`: Lower bound ($x_j \ge l$).
  - `UP`: Upper bound ($x_j \le u$).
  - `FX`: Fixed variable ($x_j = b$).
  - `FR`: Free variable ($-\infty < x_j < \infty$).
  - `MI`: Minus infinity lower bound ($x_j \ge -\infty$).
  - `PL`: Plus infinity upper bound ($x_j \le +\infty$).
  - `BV`: Binary variable ($x_j \in \{0, 1\}$).
  - `LI`: Integer variable lower bound ($x_j \ge l$, integer).
  - `UI`: Integer variable upper bound ($x_j \le u$, integer).
- **`ENDATA`**: Signals end of instance definition.

#### Robust Diagnostics:
- Rejects malformed numeric tokens, unknown row types, unclosed integer markers, non-existent variable references, and truncated files without `ENDATA`.

---

## 4. Netlib & MIPLIB Benchmark Suites

### Directory Structure:
- `benchmarks/netlib/`: Contains representative Netlib LP instances (`afiro.mps`, `adlittle.mps`, `share2b.mps`).
- `benchmarks/miplib/`: Contains representative MIPLIB MILP instances (`blend2.mps`, `p0033.mps`).

---

## 5. HiGHS Oracle Adapter Architecture

### `HighsOracleAdapter`: `include/bharatopt/highs_oracle_adapter.hpp`
- Invokes external HiGHS executable via command line (`highs --model_file=...`) or reads cached oracle solution reference files.
- Captures status, objective value, solve time, and primal solution for independent comparative analysis.
- Reports `NOT_AVAILABLE` cleanly if external HiGHS binary is not installed on host.

---

## 6. Testing & Regression Strategy

### Dedicated Phase 21 Unit Tests (`tests/test_mps_parser.cpp` & `tests/test_benchmark_framework.cpp`):
1. LP MPS parsing (afiro, adlittle).
2. MILP MPS parsing (integer markers, binary bounds).
3. Bounds parsing (`LO`, `UP`, `FX`, `FR`, `MI`, `PL`, `BV`, `LI`, `UI`).
4. Ranged constraints parsing (`RANGES` section).
5. Malformed MPS handling (missing `ENDATA`, invalid row, bad numeric value).
6. Netlib instance suite loading.
7. MIPLIB instance suite loading.
8. HiGHS oracle adapter isolation & reporting.
9. Benchmark execution modes (`SYNTHETIC`, `NETLIB`, `MIPLIB`).
10. CSV / JSON result export and import.
11. Integration with Phase 20 `SolutionVerifier`.

### Full Suite Regression:
- All 365 previous tests plus new Phase 21 tests must pass (100% pass rate) in both Debug and Release builds.

---
*Approved plan for Phase 21.*
