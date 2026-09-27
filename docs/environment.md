# BharatOpt — System Environment Specifications

**Document Version:** 1.0.0  
**Date:** September 25, 2026  
**Status:** Inspected & Verified  

---

## 1. Host Hardware Specifications

| Component | Specification |
| :--- | :--- |
| **Operating System** | Microsoft Windows 11 Home Single Language 64-bit (Build 10.0.26200) |
| **CPU Architecture** | x86_64 / x64 |
| **Processor** | 13th Gen Intel(R) Core(TM) i5-13420H |
| **CPU Cores / Threads** | 8 Physical Cores (4 Performance-Cores + 4 Efficient-Cores) / 12 Logical Processors |
| **Max CPU Frequency** | ~4.60 GHz Max Turbo |
| **Total System Memory (RAM)** | 16.0 GB DDR4/DDR5 |
| **Primary Storage (C:)** | NVMe SSD — 13.5 GB Available (System Drive) |
| **Secondary Storage (D: / E:)**| NVMe SSD — D: 80.5 GB Available / E: 102.2 GB Available |

---

## 2. GPU Acceleration Hardware

| Parameter | Specification |
| :--- | :--- |
| **Discrete GPU** | NVIDIA GeForce RTX 2050 Laptop GPU |
| **Architecture** | Ampere Architecture (GA107) |
| **Compute Capability** | SM 8.6 |
| **Dedicated VRAM** | 4096 MiB (4 GB) GDDR6 |
| **Memory Bus Width** | 64-bit |
| **Graphics Driver Version** | 592.82 (WDDM 3.1) |
| **Max Supported CUDA Version**| CUDA 13.1 (Driver API Version: 13.1) |
| **Integrated GPU** | Intel(R) UHD Graphics (128 MB Dedicated, Shared System Memory) |

---

## 3. Software Environment & Tooling Audit

| Tool / Subsystem | Detected State | Action Plan / Provisioning Strategy |
| :--- | :--- | :--- |
| **Package Manager** | `winget` 1.10+ Available | Primary package retriever for dependencies |
| **C++ Compiler** | MinGW GCC 6.3.0 (32-bit legacy) | Upgrade to MSVC 2022 / GCC 13+ (MinGW-w64 UCRT64) / LLVM Clang 18 in Phase 1 |
| **CUDA Toolkit (`nvcc`)** | Driver 592.82 Active; Toolkit pending | Install CUDA Toolkit 12.x / 13.x in Phase 1 for `nvcc` compilation |
| **Build System (`cmake`)** | Pending PATH registration | Install CMake 3.28+ via `winget` / MSVC Build Tools in Phase 1 |
| **C++ Standard Target** | C++17 / C++20 | Enabled across all targets with strict modern standards compliance |
| **Testing Framework** | GoogleTest / Catch2 | Single-header Catch2 v3 or CMake `FetchContent` GoogleTest in Phase 1 |
| **Benchmarking Framework** | Custom Telemetry / Google Benchmark | High-resolution CPU `std::chrono` + CUDA Event Timer Telemetry Engine |
| **Profiling Tools** | Available on Host | NVIDIA Nsight Systems, Nsight Compute, Visual Studio Profiler |

---

## 4. Operational Boundaries & Hardware Constraints

1. **VRAM Capacity Boundary (4 GB GDDR6):** Large-scale GPU PDHG matrices must fit within ~3.5 GB VRAM after allocation overheads. Streaming / mini-batching mechanisms will be evaluated for massive instances exceeding VRAM bounds.
2. **CPU Threading Boundary (12 Threads):** OpenMP / `std::jthread` pool target set to 8-12 threads maximum to avoid over-subscription.
3. **Storage Boundary:** Project source code, build output, and benchmark artifacts will be stored cleanly in the `BharatOpt` repository root.

---
*Verified by Phase 0 Inspection Tooling.*
