cmake_minimum_required(VERSION 3.20)

project(BharatOpt
    VERSION 0.1.0
    DESCRIPTION "Adaptive Indigenous CPU-GPU Optimisation Solver for LP and MILP"
    LANGUAGES CXX C
)

# C++ Standard Configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release build type if none specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# Compiler Warning Flags
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include Directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(bharatopt_core STATIC src/lp_model.cpp src/model_validator.cpp src/sparse_matrix.cpp src/presolve.cpp src/educational_simplex.cpp src/revised_simplex.cpp src/sparse_lu.cpp src/dual_revised_simplex.cpp src/basis_update.cpp src/gpu_backend.cpp src/first_order_solver.cpp src/benchmark_framework.cpp src/cost_estimator.cpp src/execution_router.cpp src/milp_foundation.cpp src/branch_and_bound.cpp src/solution_verifier.cpp src/mps_parser.cpp src/highs_oracle_adapter.cpp src/ablation_study.cpp src/scalability_test.cpp src/numerical_stress_test.cpp src/final_benchmark.cpp src/cli_dashboard.cpp)
target_include_directories(bharatopt_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(WIN32)
    target_link_libraries(bharatopt_core PUBLIC psapi)
endif()

# CLI Executable Target
add_executable(bharatopt_cli src/main.cpp)
target_link_libraries(bharatopt_cli PRIVATE bharatopt_core)

# Subdirectories
enable_testing()
add_subdirectory(tests)
add_subdirectory(benchmarks)
add_subdirectory(examples)
