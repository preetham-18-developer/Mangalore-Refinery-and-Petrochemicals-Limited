* ============================================================
* PROCUREMENT_LP - Test MPS file (FIXED)
* Pure continuous LP. Previous version had RANGES on the
* SLOT*CAP rows that forced a combined minimum of 1150 units
* across slots, contradicting TOTFARM = 1000 (equality row) ->
* infeasible. Ranges removed; slots are now plain <= caps.
* Integer MARKER block also removed (this is an LP, not a MIP).
* ============================================================
NAME          PROCUREMENT_LP
ROWS
 N  COST
 L  SLOT1CAP
 L  SLOT2CAP
 L  SLOT3CAP
 L  SLOT4CAP
 G  CTRDEM1
 G  CTRDEM2
 G  CTRDEM3
 G  CTRDEM4
 G  CTRDEM5
 E  TOTFARM
 L  BUDGET
 L  PEAKSHARE
COLUMNS
    X1_1      COST         12.0   SLOT1CAP      1.0
    X1_1      CTRDEM1       1.0   TOTFARM       1.0
    X1_1      BUDGET       12.0   PEAKSHARE     1.0
    X1_2      COST         15.0   SLOT2CAP      1.0
    X1_2      CTRDEM1       1.0   TOTFARM       1.0
    X1_2      BUDGET       15.0
    X1_3      COST         20.0   SLOT3CAP      1.0
    X1_3      CTRDEM1       1.0   TOTFARM       1.0
    X1_3      BUDGET       20.0
    X1_4      COST         18.0   SLOT4CAP      1.0
    X1_4      CTRDEM1       1.0   TOTFARM       1.0
    X1_4      BUDGET       18.0
    X2_1      COST         10.0   SLOT1CAP      1.0
    X2_1      CTRDEM2       1.0   TOTFARM       1.0
    X2_1      BUDGET       10.0   PEAKSHARE     1.0
    X2_2      COST         11.0   SLOT2CAP      1.0
    X2_2      CTRDEM2       1.0   TOTFARM       1.0
    X2_2      BUDGET       11.0
    X2_3      COST         14.0   SLOT3CAP      1.0
    X2_3      CTRDEM2       1.0   TOTFARM       1.0
    X2_3      BUDGET       14.0
    X2_4      COST         16.0   SLOT4CAP      1.0
    X2_4      CTRDEM2       1.0   TOTFARM       1.0
    X2_4      BUDGET       16.0
    X3_1      COST         20.0   SLOT1CAP      1.0
    X3_1      CTRDEM3       1.0   TOTFARM       1.0
    X3_1      BUDGET       20.0   PEAKSHARE     1.0
    X3_2      COST         18.0   SLOT2CAP      1.0
    X3_2      CTRDEM3       1.0   TOTFARM       1.0
    X3_2      BUDGET       18.0
    X3_3      COST         15.0   SLOT3CAP      1.0
    X3_3      CTRDEM3       1.0   TOTFARM       1.0
    X3_3      BUDGET       15.0
    X3_4      COST         12.0   SLOT4CAP      1.0
    X3_4      CTRDEM3       1.0   TOTFARM       1.0
    X3_4      BUDGET       12.0
    X4_1      COST         14.0   SLOT1CAP      1.0
    X4_1      CTRDEM4       1.0   TOTFARM       1.0
    X4_1      BUDGET       14.0   PEAKSHARE     1.0
    X4_2      COST         13.0   SLOT2CAP      1.0
    X4_2      CTRDEM4       1.0   TOTFARM       1.0
    X4_2      BUDGET       13.0
    X4_3      COST         12.0   SLOT3CAP      1.0
    X4_3      CTRDEM4       1.0   TOTFARM       1.0
    X4_3      BUDGET       12.0
    X4_4      COST         11.0   SLOT4CAP      1.0
    X4_4      CTRDEM4       1.0   TOTFARM       1.0
    X4_4      BUDGET       11.0
    X5_1      COST         16.0   SLOT1CAP      1.0
    X5_1      CTRDEM5       1.0   TOTFARM       1.0
    X5_1      BUDGET       16.0   PEAKSHARE     1.0
    X5_2      COST         17.0   SLOT2CAP      1.0
    X5_2      CTRDEM5       1.0   TOTFARM       1.0
    X5_2      BUDGET       17.0
    X5_3      COST         15.0   SLOT3CAP      1.0
    X5_3      CTRDEM5       1.0   TOTFARM       1.0
    X5_3      BUDGET       15.0
    X5_4      COST         14.0   SLOT4CAP      1.0
    X5_4      CTRDEM5       1.0   TOTFARM       1.0
    X5_4      BUDGET       14.0
RHS
    RHS       SLOT1CAP    400.0   SLOT2CAP    350.0
    RHS       SLOT3CAP    300.0   SLOT4CAP    250.0
    RHS       CTRDEM1     150.0   CTRDEM2     120.0
    RHS       CTRDEM3     180.0   CTRDEM4     100.0
    RHS       CTRDEM5     140.0   TOTFARM    1000.0
    RHS       BUDGET    14000.0   PEAKSHARE   380.0
BOUNDS
 UP BND       X1_1         90.0
 UP BND       X1_2         90.0
 UP BND       X1_3         90.0
 UP BND       X1_4         90.0
 UP BND       X2_1         80.0
 UP BND       X2_2         80.0
 UP BND       X2_3         80.0
 UP BND       X2_4         80.0
 UP BND       X3_1        100.0
 UP BND       X3_2        100.0
 UP BND       X3_3        100.0
 UP BND       X3_4        100.0
 UP BND       X4_1         70.0
 UP BND       X4_2         70.0
 UP BND       X4_3         70.0
 UP BND       X4_4         70.0
 UP BND       X5_1         85.0
 UP BND       X5_2         85.0
 UP BND       X5_3         85.0
 UP BND       X5_4         85.0
 LO BND       X1_1          5.0
 LO BND       X2_1          5.0
 LO BND       X3_1          5.0
 LO BND       X4_1          5.0
 LO BND       X5_1          5.0
ENDATA
