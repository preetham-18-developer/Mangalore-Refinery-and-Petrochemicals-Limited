# BharatOpt — Problem Requirements & Technical Specifications

**Document Version:** 1.0.0  
**Target Event:** Smart India Hackathon (SIH) 2026  
**Problem Statement ID:** PS 26119  
**Title:** Indigenous GPU-Accelerated Optimization Solver  
**Organization:** Mangalore Refinery and Petrochemicals Limited (MRPL)  
**Lead Architecture:** BharatOpt — Adaptive Indigenous CPU–GPU Optimisation Solver  

---

## 1. Problem Context & SIH Mandate

Industrial optimization models in petroleum refining, supply chain logistics, blend scheduling, and resource allocation involve massive Linear Programming (LP) and Mixed Integer Linear Programming (MILP) models containing hundreds of thousands of constraints and variables. Existing commercial solvers (Gurobi, CPLEX, Xpress) are proprietary, costly, closed-source, and subject to foreign licensing restrictions. Existing open-source solvers (HiGHS, SCIP, CBC) are primarily CPU-bound and do not natively leverage modern GPU hardware for massive-scale parallel first-order optimization.

**MRPL SIH PS 26119 Requirement:**  
Develop an **indigenous, high-performance optimization solver** capable of solving large-scale LP and MILP problems using hybrid CPU-GPU computing architectures without external solver binary dependencies inside the solving engine.

---

## 2. Core Functional Requirements

### REQ-01: Strict Anti-Wrapper Architecture
- **Mandate:** BharatOpt must implement its own algorithms directly from mathematical principles in C++ and CUDA.
- **Forbidden Dependencies:** CPLEX, Gurobi, Xpress, HiGHS, CBC, SCIP, GLPK (or any third-party optimization engine) **MUST NOT** be used as an internal solving engine.
- **Permitted External Tooling:** External open-source solvers (such as HiGHS) may ONLY be invoked externally in benchmarking tools or test scripts as an independent validation oracle to verify solution correctness, objective matching, and performance comparison.

### REQ-02: Mathematical LP & MILP Data Representation
- Support Primal Linear Programs:
  $$\min \quad c^T x \quad \text{or} \quad \max \quad c^T x$$
  $$\text{subject to} \quad A x \le b \quad (\text{or } Ax = b, Ax \ge b)$$
  $$l \le x \le u$$
- Continuous variables ($x_j \in \mathbb{R}$), integer variables ($x_j \in \mathbb{Z}$), binary variables ($x_j \in \{0, 1\}$).
- General bounds including free variables ($-\infty, +\infty$), non-negative bounds, and bounded ranges.

### REQ-03: Sparse Matrix Computing Engine
- Zero-based indexing memory-efficient sparse matrix representations:
  - **COO (Coordinate Format):** Dynamic construction & input assembly.
  - **CSR (Compressed Sparse Row):** Efficient row-wise operations & GPU SpMV.
  - **CSC (Compressed Sparse Column):** Fast column pricing & Simplex column access.
- Low-overhead conversion routines: $\text{COO} \to \text{CSR}$, $\text{COO} \to \text{CSC}$.
- Parallel matrix-vector multiplication kernels ($y = Ax$ and $y = A^T x$) with strict mathematical error checking against dense reference implementations.

### REQ-04: Advanced Presolve Engine
- Reduce model size before passing to core numerical solvers while preserving exact mathematical equivalence.
- Presolve transformations required:
  1. Fixed variable elimination.
  2. Empty row & column removal.
  3. Bound tightening via constraint propagation.
  4. Singleton row & column reductions.
  5. Simple redundant constraint elimination.
- **Postsolve Recovery:** Full bidirectional mapping ($x_{\text{original}} \leftarrow \text{Postsolve}(x_{\text{reduced}})$) to reconstruct original variable values and Dual multipliers.

### REQ-05: Exact Simplex Solving Engine (CPU)
- **Revised Simplex Method:** Maintain basis matrix $B$ using factorisation $B = LU$. Solve $B y = b$ and $B^T y = c_B$ via forward/backward substitution rather than computing dense $B^{-1}$.
- **Fill-Reducing Ordering:** Approximate Minimum Degree (AMD) ordering to minimize LU fill-in.
- **Dual Revised Simplex Method:** Hyper-sparse basis updates (Huangfu & Hall techniques) for fast dual pivoting, warm-starting from modified bounds, and branch-and-bound relaxation solves.

### REQ-06: GPU Acceleration & First-Order PDHG Solver
- **Primal-Dual Hybrid Gradient (PDHG / PDLP):** Scalable first-order method for large LPs.
- **CUDA CSR SpMV:** Custom CUDA kernel for high-throughput $A x$ and $A^T y$ operations.
- **Vector Kernels:** CUDA kernels for vector addition, scaling, projection onto bounds $l \le x \le u$, and residual norm computations.
- **Host-Device Transfer Minimization:** Keep all iterative vector updates on GPU VRAM to eliminate PCIe bottleneck during solve iterations.

### REQ-07: Adaptive CPU–GPU Problem Router
- Dynamic profiling engine (`ProblemProfiler`) to evaluate problem attributes:
  - Matrix dimensions ($m, n$)
  - Non-zero count ($\text{NNZ}$) and density ($\frac{\text{NNZ}}{m \times n}$)
  - Integrality constraints (LP vs MILP)
  - Hardware state (VRAM availability, transfer overhead)
- **Crossover Point Routing:** Automatically route small/sparse or ill-conditioned LPs to Dual Revised Simplex (CPU) and massive sparse LPs to PDHG (GPU).

### REQ-08: MILP Branch-and-Bound Engine
- Solve mixed-integer models via LP relaxations.
- CPU-managed search tree (node queue, pseudocost branching, bound propagation, incumbent tracking, pruning).
- Integration with Dual Revised Simplex warm-starting for rapid node processing.

### REQ-09: Independent Verification Engine
- Isolated validator (`Verifier`) operating outside solver internals.
- Verify feasibility: $\|Ax - b\|_\infty \le \epsilon_{\text{feas}}$, $l_j - \epsilon \le x_j \le u_j + \epsilon$.
- Verify integrality: $|x_j - \lfloor x_j \rceil| \le \epsilon_{\text{int}}$ for $j \in \mathcal{I}$.
- Verify objective value matches $c^T x$ within $\epsilon_{\text{obj}}$.

### REQ-10: Benchmarking & Telemetry Suite
- Comprehensive telemetry recording: instance name, $(m, n, \text{NNZ})$, density, selected path (CPU Simplex vs GPU PDHG), presolve reduction statistics, solve time (ms), iterations, memory usage, residual norms, objective value, and verification status.
- Comparative evaluation against external oracles (e.g., HiGHS) on Netlib / MIPLIB benchmark sets.

---

## 3. Non-Functional Requirements & Development Philosophy

1. **Correctness First:** Mathematical and numerical correctness strictly precedes any optimization effort.
2. **Empirical Benchmarking:** No performance claim will be stated without empirical micro-benchmarking and profiling data.
3. **Gate-Based Development:** Every development phase must pass a mandatory 9-point Step Gate before proceeding to subsequent phases. Failure at any gate requires an immediate halt, minimal reproducible failure isolation, and regression verification.
4. **Code Quality:** Portable C++17/C++20, clean object ownership, zero memory leaks, zero compiler warnings.

---
*Extracted and formalized for SIH 2026 PS 26119.*
