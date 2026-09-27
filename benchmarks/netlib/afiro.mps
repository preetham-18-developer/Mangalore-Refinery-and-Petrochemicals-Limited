NAME          afiro
ROWS
 N  COST
 L  R09
 L  R10
 L  X05
 E  R12
 E  R13
 COLUMNS
    X01       COST      -.4       R09       1.
    X02       COST      -1.       R10       1.
    X03       COST      -.4       X05       1.
    X04       R09       -1.       R12       1.
    X06       R10       -1.       R13       1.
 RHS
    B         R09       80.       R10       100.
    B         X05       50.
 BOUNDS
 UP BND       X01       100.
 UP BND       X02       100.
 UP BND       X03       100.
 ENDATA
