# Phase 20 Plan — Independent Solution Verifier

## 1. Overview
Phase 20 introduces a standalone, solver-independent mathematical **`SolutionVerifier`** for BharatOpt. The verifier operates directly on candidate primal solutions against the **original** `LPModel`. It evaluates bounds, constraint activities, variable integrality, and objective consistency without relying on or trusting the solver's internal status flags or reported objective values.

---

## 2. Architectural Audit & Reusable Components

### Existing Verification Logic:
- Solver-level helper methods (`RevisedSimplex::verify_solution_feasibility`, `EducationalSimplex::verify_solution_feasibility`, `DualRevisedSimplex::verify_solution_feasibility`).
- Integer verification helper (`MilpFoundation::verify_integer_feasibility`).
- Postsolve feasibility checker (`Postsolve::verify_original_feasibility`).

### Gaps & Limitations of Current Code:
- Existing solver methods return a simple `bool` without detailed residual diagnostics, violation counters, or objective recomputation.
- Integrality check was separated from bound/constraint feasibility checks in LP solvers.
- Solver status (e.g. `OPTIMAL`) was implicitly trusted in execution routing before solution verification.

### Target Architecture:
- Create a dedicated, unified header `include/bharatopt/solution_verifier.hpp` and source `src/solution_verifier.cpp`.
- Provide `SolutionVerifier` class with comprehensive `VerificationOptions` and `VerificationResult` output.
- Update solver integration points (`ExecutionRouter`, `DualRevisedSimplex`, `RevisedSimplex`, `EducationalSimplex`, `BranchAndBoundEngine`) to leverage `SolutionVerifier`.

---

## 3. Class & Struct Interface Design

```cpp
namespace bharatopt {

struct VerificationOptions {
    real_t feasibility_tolerance{1e-4};
    real_t integrality_tolerance{1e-5};
    real_t objective_tolerance{1e-4};
    bool check_objective{true};
};

struct VerificationResult {
    bool verified{false};
    std::string status_message;

    bool has_reported_objective{false};
    real_t objective_reported{0.0};
    real_t objective_recomputed{0.0};
    real_t objective_difference{0.0};

    real_t maximum_constraint_violation{0.0};
    real_t maximum_lower_bound_violation{0.0};
    real_t maximum_upper_bound_violation{0.0};
    real_t maximum_integrality_violation{0.0};

    size_t violated_constraint_count{0};
    size_t violated_bound_count{0};
    size_t violated_integrality_count{0};

    size_t variable_count{0};
    size_t constraint_count{0};

    std::vector<std::string> diagnostic_messages;

    std::string to_string() const;
};

class SolutionVerifier {
public:
    explicit SolutionVerifier(VerificationOptions options = VerificationOptions());

    VerificationResult verify(const LPModel& model,
                              const std::vector<real_t>& candidate_solution,
                              const VerificationOptions& options = VerificationOptions()) const;

    VerificationResult verify(const LPModel& model,
                              const std::vector<real_t>& candidate_solution,
                              real_t reported_objective,
                              const VerificationOptions& options = VerificationOptions()) const;

    static bool verify_solution_feasibility(const LPModel& model,
                                            const std::vector<real_t>& candidate_solution,
                                            real_t tol = 1e-4);
};

} // namespace bharatopt
```

---

## 4. Verification Rules & Numerical Checks

1. **Dimension Compatibility:** `candidate_solution.size() == model.num_variables()`. Discrepancies fail immediately with diagnostic logs.
2. **NaN / Inf Filtering:** Any non-finite value in $x_j$ triggers immediate failure and sets violation counters.
3. **Variable Bound Checks:**
   - Lower bound: $x_j \ge l_j - \text{tol}_{\text{feas}}$
   - Upper bound: $x_j \le u_j + \text{tol}_{\text{feas}}$
   - Max lower/upper bound violations tracked independently.
4. **Constraint Activity Recomputation:**
   - Compute $a_i = \sum_{j} A_{ij} x_j$ from original model terms.
   - $\le$: $a_i \le b_i + \text{tol}_{\text{feas}}$
   - $\ge$: $a_i \ge b_i - \text{tol}_{\text{feas}}$
   - $=$: $|a_i - b_i| \le \text{tol}_{\text{feas}}$
   - RANGED: $b_i - \text{tol}_{\text{feas}} \le a_i \le \text{range\_upper} + \text{tol}_{\text{feas}}$
5. **Integrality Verification:**
   - For `INTEGER` and `BINARY` variables: $|x_j - \text{round}(x_j)| \le \text{tol}_{\text{integ}}$.
   - No automatic rounding or solution repairing.
6. **Objective Recomputation:**
   - Authoritative objective: $c^T x + \text{obj\_offset}$.
   - If candidate provides reported objective, check $|c^T x + \text{obj\_offset} - \text{obj}_{\text{reported}}| \le \text{tol}_{\text{obj}}$.

---

## 5. Testing & Regression Strategy

### Dedicated Phase 20 Unit Tests (`tests/test_solution_verifier.cpp`):
- **Bounds:** Valid bounds, lower violation, upper violation, fixed var, free var.
- **Constraints:** $\le$, $\ge$, $=$, ranged, multiple constraints, sparse models.
- **Objective:** Minimization, maximization, obj offset, objective mismatch, zero objective.
- **Integrality:** Valid integer, fractional integer, binary 0, binary 1, fractional binary, out-of-range binary.
- **Numerical & Edge Cases:** NaN, Inf, dimension mismatch, empty model, zero variables, zero constraints.
- **Independence:** Fake OPTIMAL status with invalid $x$, fake objective, fake integrality.
- **Solver Integration:** Educational Simplex, Revised Simplex, Dual Revised Simplex, First-Order / GPU, MILP B&B.
- **Presolve/Postsolve:** Original model verification post-postsolve mapping.

### Full Suite Regression:
- Ensure all 328 previous unit tests continue to pass in Debug and Release builds.

---
*Approved plan for Phase 20.*
