* ============================================================
* EXAMPLE3 - Simple LP model with singleton constraints
* Min X subject to X >= 5 (MIN5) and X <= 10 (MAX10)
* Optimal solution: X = 5.0, objective = 5.0
* ============================================================
NAME          EXAMPLE3
ROWS
 N  COST
 G  MIN5
 L  MAX10
COLUMNS
    X         COST          1.0   MIN5          1.0
    X         MAX10         1.0
RHS
    RHS       MIN5          5.0   MAX10        10.0
BOUNDS
ENDATA
