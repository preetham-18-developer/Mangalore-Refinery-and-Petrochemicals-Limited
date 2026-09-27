# Phase 24 Discovery — Numerical Stress Tests

## Original Roadmap Definition
According to the master roadmap ([docs/master-plan.md](file:///c:/Users/PREETHAM/OneDrive/%E3%83%89%E3%82%AD%E3%83%A5%E3%83%A1%E3%83%B3%E3%83%88/Desktop/Algorithm-Project/docs/master-plan.md) L117) and testing strategy ([docs/testing-strategy.md](file:///c:/Users/PREETHAM/OneDrive/%E3%83%89%E3%82%AD%E3%83%A5%E3%83%A1%E3%83%B3%E3%83%88/Desktop/Algorithm-Project/docs/testing-strategy.md) Section 2 Tier 5):
- **Phase Title:** Phase 24 — Numerical Stress Tests
- **Category:** Tier 5 Numerical Quality Assurance & Boundary Evaluation
- **Defined Location:** `docs/gates/phase-24-gate.md`

## Original Objective
Empirically evaluate and benchmark BHARATOPT solver stability, precision degradation, pivot tolerance mechanisms, presolve scaling, and failure recovery under ill-conditioned, degenerate, unbounded, infeasible, and extreme coefficient numerical stress workloads.

## Original Deliverables
1. **Numerical Stress Test Harness:**
   - Ill-conditioned basis matrix generator (condition numbers $\kappa(B) \ge 10^8$, Hilbert/Vandermonde-like structures).
   - Degenerate LP generator (multiple zero reduced costs, anti-cycling / Bland's rule validation).
   - Unbounded LP generator ($c^T x \to -\infty$, ray detection).
   - Infeasible LP generator ($Ax \le b$ infeasible, dual ray / Phase I infeasibility proof).
   - Extreme coefficient scale generator (huge coefficients $10^9$ down to tiny non-zeros $10^{-12}$).
2. **Numerical Telemetry Suite:**
   - Machine-readable telemetry exports: `phase-24-numerical-stress.csv` and `phase-24-numerical-stress.json`.
   - Metrics: condition number estimate, maximum constraint residual, relative objective error, pivot status, refactorization count, fallback count, execution status.
3. **Dedicated Unit Test Suite:**
   - `tests/test_numerical_stress.cpp` verifying stress generators, tolerance handling, failure recovery, and telemetry exports.
4. **Documentation Artifacts:**
   - `docs/phase-24-report.md`
   - `docs/gates/phase-24-gate.md`

## Original Non-Goals
- Do NOT introduce new solver core architectures or rewrite existing Simplex / PDHG solvers.
- Do NOT swallow numerical errors or mask bad pivots with dummy return values.
- Do NOT modify Phase 21 benchmark framework or Phase 22 ablation harnesses.
- Do NOT assume external commercial solvers (Gurobi, CPLEX) exist as internal dependencies.

## Dependencies
- **Phase 4:** Sparse matrix data structures (`COOMatrix`, `CSCMatrix`).
- **Phase 7 & 9:** Primal and Dual Revised Simplex solvers (`RevisedSimplex`, `DualRevisedSimplex`).
- **Phase 8:** Sparse LU factorisation & pivoting logic.
- **Phase 20:** Independent solution verifier (`SolutionVerifier`).
- **Phase 21–23:** Telemetry schemas, ablation pipelines, and scalability measurement patterns.

## Current Repository Status
- **Verified Regression Baseline:** 402 / 402 PASS across Release and Debug builds with 0 compiler warnings and 0 errors.
- **Phase 24 State:** NOT IMPLEMENTED (Discovery phase only).

## Existing Related Implementation
| Component | Existing Implementation | File Reference | Status |
|---|---|---|---|
| Independent Verifier | Feasibility, residual, bound, and integrality checks | `src/solution_verifier.cpp` | COMPLETE (Phase 20) |
| Basic Degeneracy / Unbounded Tests | Unit tests for single unbounded or degenerate LPs | `tests/test_revised_simplex.cpp` | PARTIAL |
| Sparse Matrix Representations | COO / CSR / CSC sparse structures | `include/bharatopt/sparse_matrix.hpp` | COMPLETE (Phase 4) |
| Numerical Stress Harness | Dedicated stress generator & harness for $\kappa(B) \ge 10^8$ | N/A | NOT IMPLEMENTED |
| Extreme Scale Generator | Coefficient scaling harness ($10^9$ to $10^{-12}$) | N/A | NOT IMPLEMENTED |
| Machine-Readable Telemetry | CSV / JSON export for Phase 24 | N/A | NOT IMPLEMENTED |
| Dedicated Test Suite | `tests/test_numerical_stress.cpp` | N/A | NOT IMPLEMENTED |

## Missing Components
1. `include/bharatopt/numerical_stress_test.hpp` — Stress workload definitions, generator interfaces, and telemetry exporter.
2. `src/numerical_stress_test.cpp` — Ill-conditioned matrix generation, extreme coefficient scaling, and CSV/JSON reporting.
3. `tests/test_numerical_stress.cpp` — Dedicated Phase 24 unit test suite.
4. `docs/phase-24-report.md` — Experimental results report.
5. `docs/gates/phase-24-gate.md` — Phase 24 gate criteria document.

## Hardware and Environment Constraints
- **Native CUDA:** `NOT_AVAILABLE`. GPU stress tests will execute via CPU fallback path (`CPU_FALLBACK`) and report `NOT_AVAILABLE` for GPU device timings.
- **External HiGHS Oracle:** `NOT_AVAILABLE`. Solution accuracy will be verified against independent `SolutionVerifier` and analytical ground truths.
- **Environment:** Windows host OS with GCC / MinGW toolchain. Standard C++ STL and `<psapi.h>` available.

## Verification Requirements
- All stress test generators must run deterministically using fixed random seeds.
- The solver must report accurate status flags (`OPTIMAL`, `INFEASIBLE`, `UNBOUNDED`, `NUMERICAL_FAILURE`, or `SKIPPED_RESOURCE_LIMIT`) without crashing or hanging.
- 100% full regression pass (402 baseline + new Phase 24 tests).
- Clean Debug and Release builds with zero warnings and zero errors.

## Recommended Implementation Order
1. **Define Telemetry Schema & Structs:** Implement `NumericalStressRecord` in `include/bharatopt/numerical_stress_test.hpp`.
2. **Implement Generators:** Create deterministic synthetic generators for ill-conditioned, degenerate, unbounded, infeasible, and extreme coefficient scale models.
3. **Implement Telemetry Reporters:** Implement CSV/JSON exporter functions writing `phase-24-numerical-stress.csv` and `phase-24-numerical-stress.json`.
4. **Create Dedicated Tests:** Write `tests/test_numerical_stress.cpp` covering generators, status handling, precision checks, and export formatting.
5. **Update Build Files:** Register `test_numerical_stress.cpp` in `tests/CMakeLists.txt`.
6. **Execute Benchmarks & Regression:** Run test suite in Debug and Release modes, verifying full regression.
7. **Generate Documentation:** Create `docs/phase-24-report.md` and `docs/gates/phase-24-gate.md`.

## Phase 24 Gate Requirements
- **STATUS:** IMPLEMENTED
- **VERIFICATION:** VERIFIED
- **DEDICATED TESTS:** PASS
- **FULL REGRESSION:** PASS (402 + Phase 24 tests)
- **DEBUG BUILD:** PASS
- **RELEASE BUILD:** PASS
- **CSV TELEMETRY:** PASS
- **JSON TELEMETRY:** PASS
- **WARNINGS:** 0
- **ERRORS:** 0

## Conclusion
Phase 24 discovery is complete. The exact scope, dependencies, current implementation gaps, and implementation plan have been documented. Phase 24 implementation will remain paused until explicitly requested.
