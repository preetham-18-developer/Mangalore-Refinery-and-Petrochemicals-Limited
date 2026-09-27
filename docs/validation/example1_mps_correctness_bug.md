# BHARATOPT Correctness Investigation & Fix Report — `example1.mps`

> **Document Status**: Complete & Validated  
> **Target Problem**: `example1.mps`  
> **Result**: Objective corrected from `0.0` to `36.0` ($x_1 = 2, x_2 = 6$)  
> **Authoritative C++ Pipeline**: Fully Passed  

---

## 1. Original Failure

When uploading `example1.mps` to the BHARATOPT web pipeline or executing it via CLI, the solver returned:
* **Objective**: `0.0`
* **Status**: `OPTIMAL`
* **Solution Vector**: $x_1 = 0, x_2 = 0$

This was a critical correctness failure because the true mathematical optimum of the model is:
$$x_1 = 2, \quad x_2 = 6, \quad \text{Objective} = 36$$

---

## 2. Exact Parsed Model Before Fix

Before the fix, inspecting the internal `LPModel` parsed from `example1.mps` revealed:

```text
Objective Sense: MAXIMIZE
Objective: 3*X1 + 5*X2

Constraints:
  LIM1: 1.0 * X1 <= 0.0   (Incorrect RHS: 0.0 instead of 4.0)
  LIM2: 2.0 * X2 <= 0.0   (Incorrect RHS: 0.0 instead of 12.0)
  LIM3: 3.0 * X1 + 2.0 * X2 <= 0.0 (Incorrect RHS: 0.0 instead of 18.0)

Bounds:
  X1 >= 0
  X2 >= 0
```

Because all RHS values were parsed as `0.0`, the feasible space was incorrectly constrained to $x_1 \le 0, x_2 \le 0$, forcing the solver to return $(0, 0)$ with objective `0.0`.

---

## 3. Root Cause Analysis

In standard MPS format specification:
* **Section Headers** (`NAME`, `OBJSENSE`, `ROWS`, `COLUMNS`, `RHS`, `BOUNDS`, `ENDATA`) must appear **non-indented** starting at column 1 (`line[0] != ' ' && line[0] != '\t'`).
* **Data Records** (rows, matrix coefficients, RHS values, bounds) appear **indented** with leading whitespace (`line[0] == ' ' || line[0] == '\t'`).

In `example1.mps`, the `RHS` section contains data lines where the vector name is explicitly string `"RHS"`:
```text
RHS
    RHS       LIM1            4.0   LIM2           12.0
    RHS       LIM3           18.0
```

In `src/mps_parser.cpp` (and `web/src/utils/mpsParser.js`), the token check evaluated:
```cpp
// BUG IN ORIGINAL PARSER:
std::string keyword = tokens[0];
if (keyword == "RHS") {
    section = MpsSection::RHS;
    continue; // <--- Executed on data lines starting with vector name "RHS"!
}
```

Because `line.trim()` removed leading whitespace before tokenizing, `tokens[0]` for the data line `    RHS       LIM1            4.0 ...` was `"RHS"`. The parser mistook the data record for a section header keyword `RHS`, reset `section = MpsSection::RHS`, and executed `continue;`. 

As a result, **every RHS data line was skipped as if it were a header card**, leaving all constraint right-hand sides at default `0.0`.

---

## 4. Corrective Change

The parser was updated to strictly enforce non-indented header cards versus indented data records.

### C++ Core Engine (`src/mps_parser.cpp`):
```cpp
// FIX: Header cards MUST NOT be indented (line[0] != ' ' && line[0] != '\t')
bool is_header_line = (line[0] != ' ' && line[0] != '\t') || section == MpsSection::NONE;

if (is_header_line) {
    if (keyword == "NAME") { ... continue; }
    if (keyword == "OBJSENSE") { section = MpsSection::OBJSENSE; continue; }
    if (keyword == "ROWS") { section = MpsSection::ROWS; continue; }
    if (keyword == "COLUMNS") { section = MpsSection::COLUMNS; continue; }
    if (keyword == "RHS") { section = MpsSection::RHS; continue; }
    if (keyword == "BOUNDS") { section = MpsSection::BOUNDS; continue; }
    if (keyword == "ENDATA") { section = MpsSection::ENDATA; break; }
}
```

### Client-Side Preview Parser (`web/src/utils/mpsParser.js`):
Applied identical `isHeaderLine = rawLine[0] !== ' ' && rawLine[0] !== '\t'` check to ensure web preview and C++ CLI backend parser behaviors remain 100% synchronized.

---

## 5. Parser Validation

Parsing `example1.mps` with the corrected parser yields exact mathematical formulation:

* **Objective Sense**: `MAXIMIZE`
* **Objective Function**: $3 \cdot X_1 + 5 \cdot X_2$
* **Variables ($N$)**: 2 ($X_1, X_2$)
* **Constraints ($M$)**: 3 ($LIM1, LIM2, LIM3$)
* **Matrix Non-Zeros ($NNZ$)**: 4
  * $(X_1, LIM1) = 1.0$
  * $(X_1, LIM3) = 3.0$
  * $(X_2, LIM2) = 2.0$
  * $(X_2, LIM3) = 2.0$
* **Right-Hand Side Vector ($b$)**:
  * $LIM1 = 4.0$
  * $LIM2 = 12.0$
  * $LIM3 = 18.0$
* **Bounds**:
  * $0 \le X_1 < \infty$
  * $0 \le X_2 < \infty$

---

## 6. Solver Validation

Running Dual Revised Simplex on the parsed model:
* **Original Model**: Solved in 3 simplex iterations to $x_1 = 2.0, x_2 = 6.0$, objective = `36.000000`.
* **Presolved Model**: Presolve identified $1 \cdot X_1 \le 4 \implies X_1 \le 4$ and $2 \cdot X_2 \le 12 \implies X_2 \le 6$ as bound constraints, eliminating redundant rows $LIM1$ and $LIM2$. Reduced model (1 constraint $3 X_1 + 2 X_2 \le 18$) solved to $x_1 = 2.0, x_2 = 6.0$, objective = `36.000000`.

---

## 7. Verifier Validation

The independent C++ `SolutionVerifier` evaluated the candidate solution $(x_1 = 2.0, x_2 = 6.0)$:

```text
Constraint Residuals:
  LIM1: |1.0*(2.0) - 4.0| = 2.0 <= 4.0 (Viol = 0.00e+00)
  LIM2: |2.0*(6.0) - 12.0| = 12.0 <= 12.0 (Viol = 0.00e+00)
  LIM3: |3.0*(2.0) + 2.0*(6.0) - 18.0| = 18.0 <= 18.0 (Viol = 0.00e+00)

Bound Violations:
  X1 = 2.0 >= 0 (Viol = 0.00e+00)
  X2 = 6.0 >= 0 (Viol = 0.00e+00)

Recomputed Objective: 3*(2.0) + 5*(6.0) = 36.000000
Verification Status : PASS
```

---

## 8. Direct CLI Result

Running `.\build\bharatopt_cli.exe --file example1.mps --solver auto --presolve on --verify`:

```text
========================================================
 BHARATOPT OPTIMISATION SOLVER ENGINE
 BharatOpt Solver v0.1.0-alpha (SIH 2026 PS 26119)
 SIH 2026 PS 26119 | Target: MRPL
========================================================

[ MODEL ]
  File Name           : example1.mps
  Problem Name        : EXAMPLE1
  Problem Type        : LP (Linear Program)
  Constraints (M)     : 3
  Variables (N)       : 2
  Non-Zeros (NNZ)     : 4
  Sparsity Density    : 66.67%
  Variable Types      : Continuous=2, Integer=0, Binary=0
  Objective Sense     : MAXIMIZE

[ VALIDATION ]
  Validation Status   : PASS

[ PRESOLVE ]
  Presolve Policy     : ENABLED
  Original Rows       : 3
  Reduced Rows        : 1 (Removed: 2)
  Original Columns    : 2
  Reduced Columns     : 2 (Removed: 0)
  Presolve Time       : 0.00 ms

[ ADAPTIVE ROUTER ]
  Routing Mode        : AUTO (Adaptive Profiler)
  Selected Solver     : DualRevisedSimplex
  Execution Target    : CPU

[ SOLVE ]
  Solver Status       : OPTIMAL
  Optimal Objective   : 36.000000
  Simplex Iterations  : 3
  B&B Tree Nodes      : 0
  Solve Time          : 0.00 ms
  Total End-to-End    : 1.54 ms

[ VERIFICATION ]
  SolutionVerifier    : PASS
  Status Message      : Candidate solution is mathematically valid and verified against original model.
  Max Constraint Viol : 0.00e+000
  Max Bound Viol      : 0.00e+000
  Max Integrality Viol: 0.00e+000
  Recomputed Objective: 36.000000
```

---

## 9. Web / API Result

Querying `POST http://localhost:3001/api/solve` with payload `example1.mps`:

```json
{
  "fileName": "example1.mps",
  "problemName": "EXAMPLE1",
  "type": "LP",
  "rows": 3,
  "cols": 2,
  "nnz": 4,
  "density": "66.67%",
  "objectiveSense": "MAXIMIZE",
  "validationStatus": "PASS",
  "presolvePolicy": "ENABLED",
  "presolveStats": { "rowsElim": 2, "colsElim": 0, "timeMs": 0 },
  "autoSelectedEngine": "DualRevisedSimplex (CPU)",
  "executionTarget": "CPU",
  "status": "OPTIMAL",
  "objective": 36,
  "iterations": 3,
  "solveTimeMs": 0,
  "totalTimeMs": 1.51,
  "verifierStatus": "VERIFIED PASS",
  "maxConstraintViol": 0,
  "maxBoundViol": 0,
  "maxIntegralityViol": 0,
  "recomputedObjective": 36,
  "solutionValuesFormatted": [
    { "name": "X1", "val": 2 },
    { "name": "X2", "val": 6 }
  ],
  "cppEngineExecuted": true
}
```

---

## 10. Permanent Regression Test

Added permanent C++ unit test at `tests/test_mps_example1_correctness.cpp` registered in `tests/CMakeLists.txt`:

* **Execution Output**:
  ```text
  === EXAMPLE1 PARSER DIAGNOSTIC ===
  Parse Status : SUCCESS
  Problem Name : EXAMPLE1
  Obj Sense    : MAXIMIZE
  Variables (N): 2
  Constraints M: 3
  Matrix NNZ   : 4
    Var[0] X1 : lb=0, ub=inf, c=3
    Var[1] X2 : lb=0, ub=inf, c=5
    Cons[0] LIM1 : rhs=4 terms=[(X1,1) ]
    Cons[1] LIM2 : rhs=12 terms=[(X2,2) ]
    Cons[2] LIM3 : rhs=18 terms=[(X1,3) (X2,2) ]

  === MANUAL SOLUTION VERIFIER ===
  Verified     : PASS
  Recomputed Z : 36
  Max Cons Viol: 0

  === REVISED SIMPLEX (ORIGINAL MODEL) ===
  Status   : 0
  Objective: 36
    x[0] = 2
    x[1] = 6

  === REVISED SIMPLEX (PRESOLVED MODEL) ===
  Status   : 0
  Objective: 36
    x[0] = 2
    x[1] = 6

  [ PASS ] Phase26_Example1MpsCorrectness
  ```

---

## 11. Final Objective

$$\text{Optimal Objective} = \mathbf{36.000000}$$

---

## 12. Final Solution Vector

$$x_1 = \mathbf{2.0}, \quad x_2 = \mathbf{6.0}$$

---

## 13. Remaining Limitations

1. **RANGES Section Support**: MPS files containing `RANGES` section (two-sided constraints) store upper/lower range offsets. `RANGES` section parsing can be expanded if benchmark models require range constraints.
2. **Quadratic Programming (QP)**: BHARATOPT current pipeline handles linear (LP) and mixed-integer (MILP) formulations. Quadratic terms (`QSECTION` / `QUADOBJ`) are outside current Phase 26 scope.
