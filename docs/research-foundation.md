# BharatOpt — Research Foundation & Mathematical Literature

**Document Version:** 1.0.0  
**Date:** September 25, 2026  
**Status:** Approved Architectural Reference  

---

## 1. Architectural Novelty Statement

> **Novelty & System Positioning:**  
> Existing optimization techniques and published literature provide the mathematical foundation for linear and mixed-integer programming. **BharatOpt** proposes an indigenous system architecture that combines sparse-first numerical methods, multiple solver paths (CPU Dual Revised Simplex and CUDA GPU PDHG), adaptive CPU/GPU routing, and independent verification, with the routing strategy and performance claims validated experimentally.

---

## 2. Established Research & Theoretical Literature

The mathematical design of BharatOpt draws directly from foundational research papers in mathematical optimization, numerical linear algebra, and parallel computing:

### 2.1 Dual Revised Simplex & Hyper-Sparsity
- **Huangfu & Hall (2018):** *"Parallelizing the Dual Revised Simplex Method"*  
  *Influence:* High-performance dual simplex architecture, parallel steep-edge pricing, and sub-problem partitioning.
- **Huangfu & Hall (2015):** *"Novel Update Techniques for the Revised Simplex Method"*  
  *Influence:* Efficient basis inverse updates (Forrest-Tomlin, Product Form of Inverse, and LU updates) avoiding full matrix refactorization at each pivot.
- **Hall & McKinnon (2005):** *"Hyper-Sparsity in the Revised Simplex Method"*  
  *Influence:* Exploiting hyper-sparse vectors where both the right-hand side $b$ and matrix operations contain very few non-zero elements.

### 2.2 Presolve & Problem Reduction
- **Andersen & Andersen (1995):** *"Presolving in Linear Programming"*  
  *Influence:* Primitive presolve operations (fixed variable elimination, bound tightening, empty row/column removal, singleton constraint handling).
- **Achterberg et al. (2020):** *"Presolve Reductions in Mixed Integer Programming"*  
  *Influence:* Advanced domain reduction and bound tightening algorithms for mixed-integer constraint systems.

### 2.3 Sparse Matrix Factorisation & Ordering
- **Amestoy, Davis & Duff (1996):** *"An Approximate Minimum Degree Ordering Algorithm"*  
  *Influence:* AMD ordering to minimize fill-in during LU matrix factorisation $B = LU$.
- **Davis (2006):** *"Direct Methods for Sparse Linear Systems"*  
  *Influence:* Data structures for Sparse LU, Sparse Forward/Backward Substitution (L-solve and U-solve).

### 2.4 First-Order Methods & GPU Optimization
- **Applegate et al. (2021):** *"Practical Large-Scale Linear Programming using Primal-Dual Hybrid Gradient"* (PDLP / Google OR-Tools)  
  *Influence:* Primal-Dual Hybrid Gradient (PDHG) algorithm, diagonal preconditioning, step-size adaptation, and restart strategies for massive LPs.
- **Lu & Yang (2023):** *"cuPDLP.jl: A GPU-Accelerated LP Solver in Julia"*  
  *Influence:* GPU parallelization patterns for PDHG, matrix-vector multiplication kernels in CUDA, and host-device memory layout optimizations.

---

## 3. Our Implementation

BharatOpt constructs a modular, production-grade C++17/C++20 codebase implementing these mathematical formulations:

1. **LP & MILP Model Core:** `LPModel`, `Variable`, `Constraint`, `Objective`, `Bounds`.
2. **Sparse Matrix Suite:** COO, CSR, and CSC sparse matrix data structures with zero-copy row/column views and validated SpMV kernels ($y = Ax, y = A^T x$).
3. **Presolve Engine:** Fast linear-time reduction rules with a bidirectional `Postsolve` mapping object.
4. **Dense & Sparse Simplex Solvers:** Educational Simplex for algorithmic sanity checks; Revised Simplex with LU factorisation and Dual Revised Simplex for node relaxations.
5. **CPU & CUDA PDHG Engine:** CPU reference PDHG solver for mathematical validation, followed by high-throughput CUDA kernels for SpMV and vector projections.
6. **MILP Search Engine:** CPU Branch-and-Bound solver managing integer node queues, bounds, and relaxation calls.
7. **Independent Verification:** Independent verifier auditing solution feasibility, integrality, and objective residuals.

---

## 4. Proposed System-Level Contribution

While individual algorithms are drawn from established literature, **BharatOpt contributes a unique, indigenous system-level integration**:

1. **Hardware-Aware Adaptive Routing Engine (`ProblemProfiler`):** Dynamically characterizes problem scale, sparsity, density, and integrality, selecting between Dual Revised Simplex (CPU) and PDHG (CUDA GPU) using experimentally derived crossover models.
2. **Integrated Verification Architecture:** Every solve result undergoes independent non-solver verification, guaranteeing numerical reliability before solution deployment.
3. **Zero-External-Solver Runtime Core:** Complete self-containment without third-party solver licensing, fulfilling the strategic mandate of SIH 2026 PS 26119.

---

## 5. Future Research Directions

- **Batched GPU Domain Propagation:** Offloading parallel constraint bound propagation across sub-trees during MILP search.
- **Learned Machine Learning Routing Model:** Replacing static crossover thresholds with a lightweight decision model trained on problem features and execution telemetry.
- **Bit-Packed Branching & Masking:** Investigating SIMD bit-wise operations for binary variable status tracking and domain masks.

---
*Maintained as the research foundation for BharatOpt.*
