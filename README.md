# BharatOpt

**Adaptive Indigenous CPU–GPU Optimisation Solver for Large-Scale LP and MILP**

Targeting **Smart India Hackathon (SIH) 2026 — Problem Statement PS 26119**  
**Organization:** Mangalore Refinery and Petrochemicals Limited (MRPL)  

---

## 🌟 Overview

**BharatOpt** is an indigenous, high-performance mathematical optimization solver built from mathematical foundations in portable C++17/C++20 and CUDA. It implements sparse numerical routines, presolve reductions, Dual Revised Simplex with sparse LU updates, first-order Primal-Dual Hybrid Gradient (PDHG) acceleration on CUDA GPUs, and an independent solution verifier.

> **Strict Anti-Wrapper Mandate:** BharatOpt contains its own internal algorithm implementation and does **NOT** rely on CPLEX, Gurobi, Xpress, HiGHS, CBC, SCIP, or GLPK as an internal computational engine.

---

## 🏗 Project Architecture & Structure

```
BharatOpt/
├── CMakeLists.txt         # Root build configuration
├── README.md              # Project documentation
├── LICENSE                # Open-source license
├── include/
│   └── bharatopt/         # Public C++ header interface
│       ├── config.hpp     # Configuration tokens & macros
│       └── version.hpp    # Version specifications
├── src/
│   ├── main.cpp           # CLI executable entry point
│   └── ...                # Engine implementation modules
├── tests/                 # GoogleTest / Catch2 unit test suite
├── benchmarks/            # High-resolution benchmark executable
├── examples/              # Usage examples & model samples
├── docs/                  # Architecture docs & phase gates
├── data/                  # Test datasets (Netlib, MIPLIB, Synthetic)
└── tools/                 # Environment scripts & verification helpers
```

---

## 🚀 Building & Running

### Requirements
- C++17 / C++20 Compliant Compiler (GCC 8+, Clang 10+, or MSVC 2019+)
- CMake 3.20+
- (Optional) CUDA Toolkit 11.8+ for GPU PDHG acceleration

### Build Steps

```bash
# Clone and enter project directory
cd Algorithm-Project

# Generate CMake build configuration (Release)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build binaries
cmake --build build --config Release

# Run Unit Tests
ctest --test-dir build -C Release --output-on-failure

# Run Benchmark Executable
./build/benchmarks/Release/bharatopt_benchmarks.exe
```

---

## 📈 Roadmap & Milestone M1 Focus

We adhere to strict gate-based development. The immediate milestone is **Milestone M1 — Working, Verified CPU LP Solver**:

1. **Phase 1:** Project Foundation *(Current)*
2. **Phase 2:** LP Data Model
3. **Phase 3:** Model Validator
4. **Phase 4:** Sparse Matrix Engine (COO/CSR/CSC & SpMV)
5. **Phase 5:** Presolve Engine
6. **Phase 6:** Educational Simplex Solver
7. **Phase 7:** Revised Simplex Solver
8. **Phase 8:** Sparse Basis Factorisation ($B = LU$, AMD ordering)
9. **Phase 9:** Dual Revised Simplex Solver $\to$ **M1 HARD GATE**

---
*Developed for SIH 2026 PS 26119 (MRPL).*
