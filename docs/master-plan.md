# BharatOpt — Master Development Plan & Architectural Roadmap

**Project Name:** BharatOpt  
**Full Title:** Adaptive Indigenous CPU–GPU Optimisation Solver for Large-Scale LP and MILP  
**Target:** Smart India Hackathon (SIH) 2026 — Problem Statement 26119 (MRPL)  
**Document Version:** 1.0.0  
**Date:** September 25, 2026  
**Current Phase:** Phase 0 — Inspection & Architecture Planning (COMPLETE)  

---

## 1. High-Level Target Architecture

```
                    +-----------------------+
                    |      INPUT MODEL      |
                    +-----------------------+
                                |
                                v
                    +-----------------------+
                    |     MODEL PARSER      |
                    +-----------------------+
                                |
                                v
                    +-----------------------+
                    |       VALIDATOR       |
                    +-----------------------+
                                |
                                v
                    +-----------------------+
                    |       PRESOLVE        |
                    +-----------------------+
                                |
                                v
                    +-----------------------+
                    | SPARSE REPRESENTATION |
                    |       CSR / CSC       |
                    +-----------------------+
                                |
                                v
                    +-----------------------+
                    |   PROBLEM ANALYSER    |
                    +-----------------------+
                                |
                                v
                    +-----------------------+
                    | ADAPTIVE ROUTER ENGINE|
                    +-----------------------+
                     /                     \
                    /                       \
                   v                         v
        +---------------------+   +---------------------+
        |      CPU PATH       |   |      GPU PATH       |
        | Dual Revised Simplex|   |     PDHG / PDLP     |
        | Sparse LU (B = LU)  |   |     Sparse SpMV     |
        | Basis Updates       |   |    Vector Kernels   |
        | MILP Branch-Bound   |   |     Batched Work    |
        +---------------------+   +---------------------+
                   \                         /
                    \                       /
                     v                     v
                    +-----------------------+
                    |       SOLUTION        |
                    +-----------------------+
                                |
                                v
                    +-----------------------+
                    | INDEPENDENT VERIFIER  |
                    +-----------------------+
                                |
                                v
                    +-----------------------+
                    |   BENCHMARK ENGINE    |
                    +-----------------------+
                                |
                                v
                    +-----------------------+
                    |       TELEMETRY       |
                    +-----------------------+
                                |
                                v
                    +-----------------------+
                    |     FINAL RESULT      |
                    +-----------------------+
```

---

## 2. Comprehensive 27-Phase Development Roadmap

| Phase | Title | Primary Focus & Deliverables | Gate Artifact |
| :---: | :--- | :--- | :--- |
| **0** | **Inspection & Planning** | Environment audit, requirement extraction, architecture design, strategy docs | `docs/gates/phase-00-gate.md` |
| **1** | **Project Foundation** | Clean directory structure, CMake configuration, C++ toolchain setup, unit test & benchmark executables | `docs/phase-01-report.md` |
| **2** | **LP Data Model** | `LPModel`, `Constraint`, `Variable`, `Objective`, `Bounds` classes, continuous variables | `docs/gates/phase-02-gate.md` |
| **3** | **Model Validator** | Dimension, index, NaN, bound mismatch detection; 20 valid + 20 invalid unit tests | `docs/gates/phase-03-gate.md` |
| **4** | **Sparse Matrix Engine** | COO, CSR, CSC dynamic data structures, fast conversions, dense-verified SpMV ($y=Ax, A^T x$) | `docs/gates/phase-04-gate.md` |
| **5** | **Presolve Engine** | Fixed vars, empty rows/cols, bound tightening, singleton constraints, bidirectional postsolve mapping | `docs/gates/phase-05-gate.md` |
| **6** | **Simplex Learning Engine**| Small educational Simplex for pivot validation, basis tracking, and degeneracy handling | `docs/gates/phase-06-gate.md` |
| **7** | **Revised Simplex (CPU)** | Primal Revised Simplex with dense & sparse linear solves ($B y = b$), pricing, ratio test | `docs/gates/phase-07-gate.md` |
| **8** | **Sparse LU Factorisation**| Sparse LU decomposition ($B = LU$), fill-reducing AMD ordering, fast L/U triangular solves | `docs/gates/phase-08-gate.md` |
| **9** | **Dual Revised Simplex** | Dual Revised Simplex solver, steep-edge pricing, hyper-sparse updates, warm-start capability | `docs/gates/phase-09-gate.md` |
| **10**| **CPU Performance Eng.** | Profiling CPU bottlenecks, cache locality optimization, allocation minimization | `docs/gates/phase-10-gate.md` |
| **11**| **PDHG CPU Reference** | First-order Primal-Dual Hybrid Gradient reference implementation, convergence checks | `docs/gates/phase-11-gate.md` |
| **12**| **CUDA Sparse SpMV** | Custom CUDA CSR SpMV kernel, memory transfer profiling (H2D, Kernel, D2H) | `docs/gates/phase-12-gate.md` |
| **13**| **GPU PDHG Engine** | Full CUDA PDHG implementation (vector operations & SpMV on GPU VRAM), CPU-GPU crossover benchmarking | `docs/gates/phase-13-gate.md` |
| **14**| **Adaptive CPU-GPU Router**| `ProblemProfiler` dynamic cost model and decision engine based on empirical crossover points | `docs/gates/phase-14-gate.md` |
| **15**| **MILP Branch-and-Bound** | Integer/binary variables, LP relaxations, CPU node queue search tree, incumbent pruning | `docs/gates/phase-15-gate.md` |
| **16**| **MILP Performance** | Pseudocost branching, node selection heuristics, bound propagation, warm starts | `docs/gates/phase-16-gate.md` |
| **17**| **GPU MILP Experiments** | Batched LP relaxation experiments on CUDA, evaluating throughput gains/tradeoffs | `docs/gates/phase-17-gate.md` |
| **18**| **SIMD Optimizations** | SIMD vectorisation for scalar numerical kernels, accuracy vs speedup evaluation | `docs/gates/phase-18-gate.md` |
| **19**| **Bit-Level Experiments** | Bit-packed representations for binary masks, flags, and search tree bookkeeping | `docs/gates/phase-19-gate.md` |
| **20**| **Independent Verifier** | Standalone post-solve verifier validating Ax <= b, bounds, integrality, and objective residuals | `docs/gates/phase-20-gate.md` |
| **21**| **Benchmark Framework** | Automated benchmark harness evaluating synthetic, Netlib, and MIPLIB problems vs HiGHS oracle | `docs/gates/phase-21-gate.md` |
| **22**| **Ablation Studies** | Controlled single-variable ablation tests (Presolve ON/OFF, Sparse/Dense, SIMD, Routing) | `docs/gates/phase-22-gate.md` |
| **23**| **Scalability Testing** | Scaling benchmarks from 100 to 1M+ variables/constraints, memory & GPU utilization curves | `docs/gates/phase-23-gate.md` |
| **24**| **Numerical Stress Tests** | Ill-conditioned, degenerate, unbounded, infeasible, and extreme coefficient stress testing | `docs/gates/phase-24-gate.md` |
| **25**| **Final Benchmark Suite** | Standardized final benchmark execution across full problem suite, documenting wins/losses | `docs/gates/phase-25-gate.md` |
| **26**| **SIH Final CLI & Dashboard**| Production CLI demonstration pipeline displaying presolve stats, routing, CPU/GPU solve, and verifier status | `docs/gates/phase-26-gate.md` |

---

## 3. Phase 0 Acceptance Criteria & Verification

To complete Phase 0, the following criteria must be satisfied:

1. **Workspace Inspection:** Complete audit of host directory, hardware, drivers, GPU capabilities, and installed compilers.
2. **Document Creation:**
   - [docs/environment.md](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/docs/environment.md)
   - [docs/requirements.md](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/docs/requirements.md)
   - [docs/research-foundation.md](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/docs/research-foundation.md)
   - [docs/benchmark-strategy.md](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/docs/benchmark-strategy.md)
   - [docs/testing-strategy.md](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/docs/testing-strategy.md)
   - [docs/master-plan.md](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/docs/master-plan.md)
   - [docs/gates/phase-00-gate.md](file:///c:/Users/PREETHAM/OneDrive/ドキュメント/Desktop/Algorithm-Project/docs/gates/phase-00-gate.md)
3. **Strict Zero-Solver-Code Compliance:** No solver C++ / CUDA implementation code written prior to Phase 1 approval.

---
*Approved as the official master plan for BharatOpt.*
