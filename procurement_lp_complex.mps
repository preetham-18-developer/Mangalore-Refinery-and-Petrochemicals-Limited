* ============================================================
* PROCUREMENT_LP_COMPLEX - highly complex test MPS file (v2)
* 6 centers x 4 slots x 3 days, fully day-indexed variables.
* Row types: N,L,G,E | RANGES on L/G/E rows
* Vars: continuous X (day-indexed), continuous OF, free ADJ,
* fixed Z, binary OPEN (via INTORG/INTEND default 0/1 bound).
* Feasibility verified by an independently re-parsed certificate
* check before delivery (see accompanying verification run).
* ============================================================
NAME          PROCUREMENT_LP_COMPLEX
ROWS
 N  COST
 L  SLOT1_1
 L  SLOT1_2
 L  SLOT1_3
 L  SLOT2_1
 L  SLOT2_2
 L  SLOT2_3
 L  SLOT3_1
 L  SLOT3_2
 L  SLOT3_3
 L  SLOT4_1
 L  SLOT4_2
 L  SLOT4_3
 G  CDEM1_1
 G  CDEM1_2
 G  CDEM1_3
 G  CDEM2_1
 G  CDEM2_2
 G  CDEM2_3
 G  CDEM3_1
 G  CDEM3_2
 G  CDEM3_3
 G  CDEM4_1
 G  CDEM4_2
 G  CDEM4_3
 G  CDEM5_1
 G  CDEM5_2
 G  CDEM5_3
 G  CDEM6_1
 G  CDEM6_2
 G  CDEM6_3
 G  WKQ1
 G  WKQ2
 G  WKQ3
 G  WKQ4
 G  WKQ5
 G  WKQ6
 E  TOTF1
 E  TOTF2
 E  TOTF3
 L  LINK1_1
 L  LINK1_2
 L  LINK1_3
 L  LINK2_1
 L  LINK2_2
 L  LINK2_3
 L  LINK3_1
 L  LINK3_2
 L  LINK3_3
 L  LINK4_1
 L  LINK4_2
 L  LINK4_3
 L  LINK5_1
 L  LINK5_2
 L  LINK5_3
 L  LINK6_1
 L  LINK6_2
 L  LINK6_3
 G  MINOPEN1
 G  MINOPEN2
 G  MINOPEN3
 L  BUDGET1
 L  BUDGET2
 L  BUDGET3
 L  PEAK1
 L  PEAK2
 L  PEAK3
COLUMNS
    X1_1_1    COST           12.0000  SLOT1_1         1.0000
    X1_1_1    CDEM1_1         1.0000  WKQ1            1.0000
    X1_1_1    TOTF1           1.0000  LINK1_1         1.0000
    X1_1_1    PEAK1           1.0000  BUDGET1        12.0000
    X1_1_2    COST           12.0000  SLOT1_2         1.0000
    X1_1_2    CDEM1_2         1.0000  WKQ1            1.0000
    X1_1_2    TOTF2           1.0000  LINK1_2         1.0000
    X1_1_2    PEAK2           1.0000  BUDGET2        12.0000
    X1_1_3    COST           12.0000  SLOT1_3         1.0000
    X1_1_3    CDEM1_3         1.0000  WKQ1            1.0000
    X1_1_3    TOTF3           1.0000  LINK1_3         1.0000
    X1_1_3    PEAK3           1.0000  BUDGET3        12.0000
    X1_2_1    COST           15.0000  SLOT2_1         1.0000
    X1_2_1    CDEM1_1         1.0000  WKQ1            1.0000
    X1_2_1    TOTF1           1.0000  LINK1_1         1.0000
    X1_2_1    BUDGET1        15.0000
    X1_2_2    COST           15.0000  SLOT2_2         1.0000
    X1_2_2    CDEM1_2         1.0000  WKQ1            1.0000
    X1_2_2    TOTF2           1.0000  LINK1_2         1.0000
    X1_2_2    BUDGET2        15.0000
    X1_2_3    COST           15.0000  SLOT2_3         1.0000
    X1_2_3    CDEM1_3         1.0000  WKQ1            1.0000
    X1_2_3    TOTF3           1.0000  LINK1_3         1.0000
    X1_2_3    BUDGET3        15.0000
    X1_3_1    COST           20.0000  SLOT3_1         1.0000
    X1_3_1    CDEM1_1         1.0000  WKQ1            1.0000
    X1_3_1    TOTF1           1.0000  LINK1_1         1.0000
    X1_3_1    BUDGET1        20.0000
    X1_3_2    COST           20.0000  SLOT3_2         1.0000
    X1_3_2    CDEM1_2         1.0000  WKQ1            1.0000
    X1_3_2    TOTF2           1.0000  LINK1_2         1.0000
    X1_3_2    BUDGET2        20.0000
    X1_3_3    COST           20.0000  SLOT3_3         1.0000
    X1_3_3    CDEM1_3         1.0000  WKQ1            1.0000
    X1_3_3    TOTF3           1.0000  LINK1_3         1.0000
    X1_3_3    BUDGET3        20.0000
    X1_4_1    COST           18.0000  SLOT4_1         1.0000
    X1_4_1    CDEM1_1         1.0000  WKQ1            1.0000
    X1_4_1    TOTF1           1.0000  LINK1_1         1.0000
    X1_4_1    BUDGET1        18.0000
    X1_4_2    COST           18.0000  SLOT4_2         1.0000
    X1_4_2    CDEM1_2         1.0000  WKQ1            1.0000
    X1_4_2    TOTF2           1.0000  LINK1_2         1.0000
    X1_4_2    BUDGET2        18.0000
    X1_4_3    COST           18.0000  SLOT4_3         1.0000
    X1_4_3    CDEM1_3         1.0000  WKQ1            1.0000
    X1_4_3    TOTF3           1.0000  LINK1_3         1.0000
    X1_4_3    BUDGET3        18.0000
    X2_1_1    COST           10.0000  SLOT1_1         1.0000
    X2_1_1    CDEM2_1         1.0000  WKQ2            1.0000
    X2_1_1    TOTF1           1.0000  LINK2_1         1.0000
    X2_1_1    PEAK1           1.0000  BUDGET1        10.0000
    X2_1_2    COST           10.0000  SLOT1_2         1.0000
    X2_1_2    CDEM2_2         1.0000  WKQ2            1.0000
    X2_1_2    TOTF2           1.0000  LINK2_2         1.0000
    X2_1_2    PEAK2           1.0000  BUDGET2        10.0000
    X2_1_3    COST           10.0000  SLOT1_3         1.0000
    X2_1_3    CDEM2_3         1.0000  WKQ2            1.0000
    X2_1_3    TOTF3           1.0000  LINK2_3         1.0000
    X2_1_3    PEAK3           1.0000  BUDGET3        10.0000
    X2_2_1    COST           11.0000  SLOT2_1         1.0000
    X2_2_1    CDEM2_1         1.0000  WKQ2            1.0000
    X2_2_1    TOTF1           1.0000  LINK2_1         1.0000
    X2_2_1    BUDGET1        11.0000
    X2_2_2    COST           11.0000  SLOT2_2         1.0000
    X2_2_2    CDEM2_2         1.0000  WKQ2            1.0000
    X2_2_2    TOTF2           1.0000  LINK2_2         1.0000
    X2_2_2    BUDGET2        11.0000
    X2_2_3    COST           11.0000  SLOT2_3         1.0000
    X2_2_3    CDEM2_3         1.0000  WKQ2            1.0000
    X2_2_3    TOTF3           1.0000  LINK2_3         1.0000
    X2_2_3    BUDGET3        11.0000
    X2_3_1    COST           14.0000  SLOT3_1         1.0000
    X2_3_1    CDEM2_1         1.0000  WKQ2            1.0000
    X2_3_1    TOTF1           1.0000  LINK2_1         1.0000
    X2_3_1    BUDGET1        14.0000
    X2_3_2    COST           14.0000  SLOT3_2         1.0000
    X2_3_2    CDEM2_2         1.0000  WKQ2            1.0000
    X2_3_2    TOTF2           1.0000  LINK2_2         1.0000
    X2_3_2    BUDGET2        14.0000
    X2_3_3    COST           14.0000  SLOT3_3         1.0000
    X2_3_3    CDEM2_3         1.0000  WKQ2            1.0000
    X2_3_3    TOTF3           1.0000  LINK2_3         1.0000
    X2_3_3    BUDGET3        14.0000
    X2_4_1    COST           16.0000  SLOT4_1         1.0000
    X2_4_1    CDEM2_1         1.0000  WKQ2            1.0000
    X2_4_1    TOTF1           1.0000  LINK2_1         1.0000
    X2_4_1    BUDGET1        16.0000
    X2_4_2    COST           16.0000  SLOT4_2         1.0000
    X2_4_2    CDEM2_2         1.0000  WKQ2            1.0000
    X2_4_2    TOTF2           1.0000  LINK2_2         1.0000
    X2_4_2    BUDGET2        16.0000
    X2_4_3    COST           16.0000  SLOT4_3         1.0000
    X2_4_3    CDEM2_3         1.0000  WKQ2            1.0000
    X2_4_3    TOTF3           1.0000  LINK2_3         1.0000
    X2_4_3    BUDGET3        16.0000
    X3_1_1    COST           20.0000  SLOT1_1         1.0000
    X3_1_1    CDEM3_1         1.0000  WKQ3            1.0000
    X3_1_1    TOTF1           1.0000  LINK3_1         1.0000
    X3_1_1    PEAK1           1.0000  BUDGET1        20.0000
    X3_1_2    COST           20.0000  SLOT1_2         1.0000
    X3_1_2    CDEM3_2         1.0000  WKQ3            1.0000
    X3_1_2    TOTF2           1.0000  LINK3_2         1.0000
    X3_1_2    PEAK2           1.0000  BUDGET2        20.0000
    X3_1_3    COST           20.0000  SLOT1_3         1.0000
    X3_1_3    CDEM3_3         1.0000  WKQ3            1.0000
    X3_1_3    TOTF3           1.0000  LINK3_3         1.0000
    X3_1_3    PEAK3           1.0000  BUDGET3        20.0000
    X3_2_1    COST           18.0000  SLOT2_1         1.0000
    X3_2_1    CDEM3_1         1.0000  WKQ3            1.0000
    X3_2_1    TOTF1           1.0000  LINK3_1         1.0000
    X3_2_1    BUDGET1        18.0000
    X3_2_2    COST           18.0000  SLOT2_2         1.0000
    X3_2_2    CDEM3_2         1.0000  WKQ3            1.0000
    X3_2_2    TOTF2           1.0000  LINK3_2         1.0000
    X3_2_2    BUDGET2        18.0000
    X3_2_3    COST           18.0000  SLOT2_3         1.0000
    X3_2_3    CDEM3_3         1.0000  WKQ3            1.0000
    X3_2_3    TOTF3           1.0000  LINK3_3         1.0000
    X3_2_3    BUDGET3        18.0000
    X3_3_1    COST           15.0000  SLOT3_1         1.0000
    X3_3_1    CDEM3_1         1.0000  WKQ3            1.0000
    X3_3_1    TOTF1           1.0000  LINK3_1         1.0000
    X3_3_1    BUDGET1        15.0000
    X3_3_2    COST           15.0000  SLOT3_2         1.0000
    X3_3_2    CDEM3_2         1.0000  WKQ3            1.0000
    X3_3_2    TOTF2           1.0000  LINK3_2         1.0000
    X3_3_2    BUDGET2        15.0000
    X3_3_3    COST           15.0000  SLOT3_3         1.0000
    X3_3_3    CDEM3_3         1.0000  WKQ3            1.0000
    X3_3_3    TOTF3           1.0000  LINK3_3         1.0000
    X3_3_3    BUDGET3        15.0000
    X3_4_1    COST           12.0000  SLOT4_1         1.0000
    X3_4_1    CDEM3_1         1.0000  WKQ3            1.0000
    X3_4_1    TOTF1           1.0000  LINK3_1         1.0000
    X3_4_1    BUDGET1        12.0000
    X3_4_2    COST           12.0000  SLOT4_2         1.0000
    X3_4_2    CDEM3_2         1.0000  WKQ3            1.0000
    X3_4_2    TOTF2           1.0000  LINK3_2         1.0000
    X3_4_2    BUDGET2        12.0000
    X3_4_3    COST           12.0000  SLOT4_3         1.0000
    X3_4_3    CDEM3_3         1.0000  WKQ3            1.0000
    X3_4_3    TOTF3           1.0000  LINK3_3         1.0000
    X3_4_3    BUDGET3        12.0000
    X4_1_1    COST           14.0000  SLOT1_1         1.0000
    X4_1_1    CDEM4_1         1.0000  WKQ4            1.0000
    X4_1_1    TOTF1           1.0000  LINK4_1         1.0000
    X4_1_1    PEAK1           1.0000  BUDGET1        14.0000
    X4_1_2    COST           14.0000  SLOT1_2         1.0000
    X4_1_2    CDEM4_2         1.0000  WKQ4            1.0000
    X4_1_2    TOTF2           1.0000  LINK4_2         1.0000
    X4_1_2    PEAK2           1.0000  BUDGET2        14.0000
    X4_1_3    COST           14.0000  SLOT1_3         1.0000
    X4_1_3    CDEM4_3         1.0000  WKQ4            1.0000
    X4_1_3    TOTF3           1.0000  LINK4_3         1.0000
    X4_1_3    PEAK3           1.0000  BUDGET3        14.0000
    X4_2_1    COST           13.0000  SLOT2_1         1.0000
    X4_2_1    CDEM4_1         1.0000  WKQ4            1.0000
    X4_2_1    TOTF1           1.0000  LINK4_1         1.0000
    X4_2_1    BUDGET1        13.0000
    X4_2_2    COST           13.0000  SLOT2_2         1.0000
    X4_2_2    CDEM4_2         1.0000  WKQ4            1.0000
    X4_2_2    TOTF2           1.0000  LINK4_2         1.0000
    X4_2_2    BUDGET2        13.0000
    X4_2_3    COST           13.0000  SLOT2_3         1.0000
    X4_2_3    CDEM4_3         1.0000  WKQ4            1.0000
    X4_2_3    TOTF3           1.0000  LINK4_3         1.0000
    X4_2_3    BUDGET3        13.0000
    X4_3_1    COST           12.0000  SLOT3_1         1.0000
    X4_3_1    CDEM4_1         1.0000  WKQ4            1.0000
    X4_3_1    TOTF1           1.0000  LINK4_1         1.0000
    X4_3_1    BUDGET1        12.0000
    X4_3_2    COST           12.0000  SLOT3_2         1.0000
    X4_3_2    CDEM4_2         1.0000  WKQ4            1.0000
    X4_3_2    TOTF2           1.0000  LINK4_2         1.0000
    X4_3_2    BUDGET2        12.0000
    X4_3_3    COST           12.0000  SLOT3_3         1.0000
    X4_3_3    CDEM4_3         1.0000  WKQ4            1.0000
    X4_3_3    TOTF3           1.0000  LINK4_3         1.0000
    X4_3_3    BUDGET3        12.0000
    X4_4_1    COST           11.0000  SLOT4_1         1.0000
    X4_4_1    CDEM4_1         1.0000  WKQ4            1.0000
    X4_4_1    TOTF1           1.0000  LINK4_1         1.0000
    X4_4_1    BUDGET1        11.0000
    X4_4_2    COST           11.0000  SLOT4_2         1.0000
    X4_4_2    CDEM4_2         1.0000  WKQ4            1.0000
    X4_4_2    TOTF2           1.0000  LINK4_2         1.0000
    X4_4_2    BUDGET2        11.0000
    X4_4_3    COST           11.0000  SLOT4_3         1.0000
    X4_4_3    CDEM4_3         1.0000  WKQ4            1.0000
    X4_4_3    TOTF3           1.0000  LINK4_3         1.0000
    X4_4_3    BUDGET3        11.0000
    X5_1_1    COST           16.0000  SLOT1_1         1.0000
    X5_1_1    CDEM5_1         1.0000  WKQ5            1.0000
    X5_1_1    TOTF1           1.0000  LINK5_1         1.0000
    X5_1_1    PEAK1           1.0000  BUDGET1        16.0000
    X5_1_2    COST           16.0000  SLOT1_2         1.0000
    X5_1_2    CDEM5_2         1.0000  WKQ5            1.0000
    X5_1_2    TOTF2           1.0000  LINK5_2         1.0000
    X5_1_2    PEAK2           1.0000  BUDGET2        16.0000
    X5_1_3    COST           16.0000  SLOT1_3         1.0000
    X5_1_3    CDEM5_3         1.0000  WKQ5            1.0000
    X5_1_3    TOTF3           1.0000  LINK5_3         1.0000
    X5_1_3    PEAK3           1.0000  BUDGET3        16.0000
    X5_2_1    COST           17.0000  SLOT2_1         1.0000
    X5_2_1    CDEM5_1         1.0000  WKQ5            1.0000
    X5_2_1    TOTF1           1.0000  LINK5_1         1.0000
    X5_2_1    BUDGET1        17.0000
    X5_2_2    COST           17.0000  SLOT2_2         1.0000
    X5_2_2    CDEM5_2         1.0000  WKQ5            1.0000
    X5_2_2    TOTF2           1.0000  LINK5_2         1.0000
    X5_2_2    BUDGET2        17.0000
    X5_2_3    COST           17.0000  SLOT2_3         1.0000
    X5_2_3    CDEM5_3         1.0000  WKQ5            1.0000
    X5_2_3    TOTF3           1.0000  LINK5_3         1.0000
    X5_2_3    BUDGET3        17.0000
    X5_3_1    COST           15.0000  SLOT3_1         1.0000
    X5_3_1    CDEM5_1         1.0000  WKQ5            1.0000
    X5_3_1    TOTF1           1.0000  LINK5_1         1.0000
    X5_3_1    BUDGET1        15.0000
    X5_3_2    COST           15.0000  SLOT3_2         1.0000
    X5_3_2    CDEM5_2         1.0000  WKQ5            1.0000
    X5_3_2    TOTF2           1.0000  LINK5_2         1.0000
    X5_3_2    BUDGET2        15.0000
    X5_3_3    COST           15.0000  SLOT3_3         1.0000
    X5_3_3    CDEM5_3         1.0000  WKQ5            1.0000
    X5_3_3    TOTF3           1.0000  LINK5_3         1.0000
    X5_3_3    BUDGET3        15.0000
    X5_4_1    COST           14.0000  SLOT4_1         1.0000
    X5_4_1    CDEM5_1         1.0000  WKQ5            1.0000
    X5_4_1    TOTF1           1.0000  LINK5_1         1.0000
    X5_4_1    BUDGET1        14.0000
    X5_4_2    COST           14.0000  SLOT4_2         1.0000
    X5_4_2    CDEM5_2         1.0000  WKQ5            1.0000
    X5_4_2    TOTF2           1.0000  LINK5_2         1.0000
    X5_4_2    BUDGET2        14.0000
    X5_4_3    COST           14.0000  SLOT4_3         1.0000
    X5_4_3    CDEM5_3         1.0000  WKQ5            1.0000
    X5_4_3    TOTF3           1.0000  LINK5_3         1.0000
    X5_4_3    BUDGET3        14.0000
    X6_1_1    COST           13.0000  SLOT1_1         1.0000
    X6_1_1    CDEM6_1         1.0000  WKQ6            1.0000
    X6_1_1    TOTF1           1.0000  LINK6_1         1.0000
    X6_1_1    PEAK1           1.0000  BUDGET1        13.0000
    X6_1_2    COST           13.0000  SLOT1_2         1.0000
    X6_1_2    CDEM6_2         1.0000  WKQ6            1.0000
    X6_1_2    TOTF2           1.0000  LINK6_2         1.0000
    X6_1_2    PEAK2           1.0000  BUDGET2        13.0000
    X6_1_3    COST           13.0000  SLOT1_3         1.0000
    X6_1_3    CDEM6_3         1.0000  WKQ6            1.0000
    X6_1_3    TOTF3           1.0000  LINK6_3         1.0000
    X6_1_3    PEAK3           1.0000  BUDGET3        13.0000
    X6_2_1    COST           16.0000  SLOT2_1         1.0000
    X6_2_1    CDEM6_1         1.0000  WKQ6            1.0000
    X6_2_1    TOTF1           1.0000  LINK6_1         1.0000
    X6_2_1    BUDGET1        16.0000
    X6_2_2    COST           16.0000  SLOT2_2         1.0000
    X6_2_2    CDEM6_2         1.0000  WKQ6            1.0000
    X6_2_2    TOTF2           1.0000  LINK6_2         1.0000
    X6_2_2    BUDGET2        16.0000
    X6_2_3    COST           16.0000  SLOT2_3         1.0000
    X6_2_3    CDEM6_3         1.0000  WKQ6            1.0000
    X6_2_3    TOTF3           1.0000  LINK6_3         1.0000
    X6_2_3    BUDGET3        16.0000
    X6_3_1    COST           19.0000  SLOT3_1         1.0000
    X6_3_1    CDEM6_1         1.0000  WKQ6            1.0000
    X6_3_1    TOTF1           1.0000  LINK6_1         1.0000
    X6_3_1    BUDGET1        19.0000
    X6_3_2    COST           19.0000  SLOT3_2         1.0000
    X6_3_2    CDEM6_2         1.0000  WKQ6            1.0000
    X6_3_2    TOTF2           1.0000  LINK6_2         1.0000
    X6_3_2    BUDGET2        19.0000
    X6_3_3    COST           19.0000  SLOT3_3         1.0000
    X6_3_3    CDEM6_3         1.0000  WKQ6            1.0000
    X6_3_3    TOTF3           1.0000  LINK6_3         1.0000
    X6_3_3    BUDGET3        19.0000
    X6_4_1    COST           17.0000  SLOT4_1         1.0000
    X6_4_1    CDEM6_1         1.0000  WKQ6            1.0000
    X6_4_1    TOTF1           1.0000  LINK6_1         1.0000
    X6_4_1    BUDGET1        17.0000
    X6_4_2    COST           17.0000  SLOT4_2         1.0000
    X6_4_2    CDEM6_2         1.0000  WKQ6            1.0000
    X6_4_2    TOTF2           1.0000  LINK6_2         1.0000
    X6_4_2    BUDGET2        17.0000
    X6_4_3    COST           17.0000  SLOT4_3         1.0000
    X6_4_3    CDEM6_3         1.0000  WKQ6            1.0000
    X6_4_3    TOTF3           1.0000  LINK6_3         1.0000
    X6_4_3    BUDGET3        17.0000
    MK_OPEN   'MARKER'                 'INTORG'
    OPEN1_1   LINK1_1     -1000.0000  MINOPEN1        1.0000
    OPEN1_2   LINK1_2     -1000.0000  MINOPEN2        1.0000
    OPEN1_3   LINK1_3     -1000.0000  MINOPEN3        1.0000
    OPEN2_1   LINK2_1     -1000.0000  MINOPEN1        1.0000
    OPEN2_2   LINK2_2     -1000.0000  MINOPEN2        1.0000
    OPEN2_3   LINK2_3     -1000.0000  MINOPEN3        1.0000
    OPEN3_1   LINK3_1     -1000.0000  MINOPEN1        1.0000
    OPEN3_2   LINK3_2     -1000.0000  MINOPEN2        1.0000
    OPEN3_3   LINK3_3     -1000.0000  MINOPEN3        1.0000
    OPEN4_1   LINK4_1     -1000.0000  MINOPEN1        1.0000
    OPEN4_2   LINK4_2     -1000.0000  MINOPEN2        1.0000
    OPEN4_3   LINK4_3     -1000.0000  MINOPEN3        1.0000
    OPEN5_1   LINK5_1     -1000.0000  MINOPEN1        1.0000
    OPEN5_2   LINK5_2     -1000.0000  MINOPEN2        1.0000
    OPEN5_3   LINK5_3     -1000.0000  MINOPEN3        1.0000
    OPEN6_1   LINK6_1     -1000.0000  MINOPEN1        1.0000
    OPEN6_2   LINK6_2     -1000.0000  MINOPEN2        1.0000
    OPEN6_3   LINK6_3     -1000.0000  MINOPEN3        1.0000
    MK_OPEN2  'MARKER'                 'INTEND'
    OF1_1     COST            0.0000
    OF1_2     COST            0.0000
    OF1_3     COST            0.0000
    OF2_1     COST            0.0000
    OF2_2     COST            0.0000
    OF2_3     COST            0.0000
    OF3_1     COST            0.0000
    OF3_2     COST            0.0000
    OF3_3     COST            0.0000
    OF4_1     COST            0.0000
    OF4_2     COST            0.0000
    OF4_3     COST            0.0000
    OF5_1     COST            0.0000
    OF5_2     COST            0.0000
    OF5_3     COST            0.0000
    OF6_1     COST            0.0000
    OF6_2     COST            0.0000
    OF6_3     COST            0.0000
    ADJ1      BUDGET1         1.0000
    ADJ2      BUDGET2         1.0000
    ADJ3      BUDGET3         1.0000
    Z         COST            1.0000  BUDGET1         5.0000
RHS
    RHS       SLOT1_1       400.0000  SLOT1_2       400.0000
    RHS       SLOT1_3       400.0000  SLOT2_1       350.0000
    RHS       SLOT2_2       350.0000  SLOT2_3       350.0000
    RHS       SLOT3_1       300.0000  SLOT3_2       300.0000
    RHS       SLOT3_3       300.0000  SLOT4_1       250.0000
    RHS       SLOT4_2       250.0000  SLOT4_3       250.0000
    RHS       CDEM1_1        30.0000  CDEM1_2        30.0000
    RHS       CDEM1_3        30.0000  CDEM2_1        20.0000
    RHS       CDEM2_2        20.0000  CDEM2_3        20.0000
    RHS       CDEM3_1        35.0000  CDEM3_2        35.0000
    RHS       CDEM3_3        35.0000  CDEM4_1        15.0000
    RHS       CDEM4_2        15.0000  CDEM4_3        15.0000
    RHS       CDEM5_1        25.0000  CDEM5_2        25.0000
    RHS       CDEM5_3        25.0000  CDEM6_1        25.0000
    RHS       CDEM6_2        25.0000  CDEM6_3        25.0000
    RHS       WKQ1          550.0000  WKQ2          400.0000
    RHS       WKQ3          600.0000  WKQ4          300.0000
    RHS       WKQ5          430.0000  WKQ6          430.0000
    RHS       TOTF1        1000.0000  TOTF2        1000.0000
    RHS       TOTF3        1000.0000  LINK1_1         0.0000
    RHS       LINK1_2         0.0000  LINK1_3         0.0000
    RHS       LINK2_1         0.0000  LINK2_2         0.0000
    RHS       LINK2_3         0.0000  LINK3_1         0.0000
    RHS       LINK3_2         0.0000  LINK3_3         0.0000
    RHS       LINK4_1         0.0000  LINK4_2         0.0000
    RHS       LINK4_3         0.0000  LINK5_1         0.0000
    RHS       LINK5_2         0.0000  LINK5_3         0.0000
    RHS       LINK6_1         0.0000  LINK6_2         0.0000
    RHS       LINK6_3         0.0000  MINOPEN1        4.0000
    RHS       MINOPEN2        4.0000  MINOPEN3        4.0000
    RHS       BUDGET1     15000.0000  BUDGET2     15000.0000
    RHS       BUDGET3     15000.0000  PEAK1         380.0000
    RHS       PEAK2         380.0000  PEAK3         380.0000
RANGES
    RNG       WKQ1           60.0000  WKQ2           60.0000
    RNG       WKQ3           60.0000  WKQ4           60.0000
    RNG       WKQ5           60.0000  WKQ6           60.0000
    RNG       TOTF1          20.0000  TOTF2          20.0000
    RNG       TOTF3          20.0000  BUDGET1      3000.0000
    RNG       BUDGET2      3000.0000  BUDGET3      3000.0000
BOUNDS
 UP BND       X1_1_1       90.0000
 LO BND       X1_1_1        2.0000
 UP BND       X1_1_2       90.0000
 LO BND       X1_1_2        2.0000
 UP BND       X1_1_3       90.0000
 LO BND       X1_1_3        2.0000
 UP BND       X1_2_1       90.0000
 UP BND       X1_2_2       90.0000
 UP BND       X1_2_3       90.0000
 UP BND       X1_3_1       90.0000
 UP BND       X1_3_2       90.0000
 UP BND       X1_3_3       90.0000
 UP BND       X1_4_1       90.0000
 UP BND       X1_4_2       90.0000
 UP BND       X1_4_3       90.0000
 UP BND       X2_1_1       85.0000
 LO BND       X2_1_1        2.0000
 UP BND       X2_1_2       85.0000
 LO BND       X2_1_2        2.0000
 UP BND       X2_1_3       85.0000
 LO BND       X2_1_3        2.0000
 UP BND       X2_2_1       85.0000
 UP BND       X2_2_2       85.0000
 UP BND       X2_2_3       85.0000
 UP BND       X2_3_1       85.0000
 UP BND       X2_3_2       85.0000
 UP BND       X2_3_3       85.0000
 UP BND       X2_4_1       85.0000
 UP BND       X2_4_2       85.0000
 UP BND       X2_4_3       85.0000
 UP BND       X3_1_1      100.0000
 LO BND       X3_1_1        2.0000
 UP BND       X3_1_2      100.0000
 LO BND       X3_1_2        2.0000
 UP BND       X3_1_3      100.0000
 LO BND       X3_1_3        2.0000
 UP BND       X3_2_1      100.0000
 UP BND       X3_2_2      100.0000
 UP BND       X3_2_3      100.0000
 UP BND       X3_3_1      100.0000
 UP BND       X3_3_2      100.0000
 UP BND       X3_3_3      100.0000
 UP BND       X3_4_1      100.0000
 UP BND       X3_4_2      100.0000
 UP BND       X3_4_3      100.0000
 UP BND       X4_1_1       75.0000
 LO BND       X4_1_1        2.0000
 UP BND       X4_1_2       75.0000
 LO BND       X4_1_2        2.0000
 UP BND       X4_1_3       75.0000
 LO BND       X4_1_3        2.0000
 UP BND       X4_2_1       75.0000
 UP BND       X4_2_2       75.0000
 UP BND       X4_2_3       75.0000
 UP BND       X4_3_1       75.0000
 UP BND       X4_3_2       75.0000
 UP BND       X4_3_3       75.0000
 UP BND       X4_4_1       75.0000
 UP BND       X4_4_2       75.0000
 UP BND       X4_4_3       75.0000
 UP BND       X5_1_1       80.0000
 LO BND       X5_1_1        2.0000
 UP BND       X5_1_2       80.0000
 LO BND       X5_1_2        2.0000
 UP BND       X5_1_3       80.0000
 LO BND       X5_1_3        2.0000
 UP BND       X5_2_1       80.0000
 UP BND       X5_2_2       80.0000
 UP BND       X5_2_3       80.0000
 UP BND       X5_3_1       80.0000
 UP BND       X5_3_2       80.0000
 UP BND       X5_3_3       80.0000
 UP BND       X5_4_1       80.0000
 UP BND       X5_4_2       80.0000
 UP BND       X5_4_3       80.0000
 UP BND       X6_1_1       95.0000
 LO BND       X6_1_1        2.0000
 UP BND       X6_1_2       95.0000
 LO BND       X6_1_2        2.0000
 UP BND       X6_1_3       95.0000
 LO BND       X6_1_3        2.0000
 UP BND       X6_2_1       95.0000
 UP BND       X6_2_2       95.0000
 UP BND       X6_2_3       95.0000
 UP BND       X6_3_1       95.0000
 UP BND       X6_3_2       95.0000
 UP BND       X6_3_3       95.0000
 UP BND       X6_4_1       95.0000
 UP BND       X6_4_2       95.0000
 UP BND       X6_4_3       95.0000
 UP BND       OF1_1       200.0000
 UP BND       OF1_2       200.0000
 UP BND       OF1_3       200.0000
 UP BND       OF2_1       200.0000
 UP BND       OF2_2       200.0000
 UP BND       OF2_3       200.0000
 UP BND       OF3_1       200.0000
 UP BND       OF3_2       200.0000
 UP BND       OF3_3       200.0000
 UP BND       OF4_1       200.0000
 UP BND       OF4_2       200.0000
 UP BND       OF4_3       200.0000
 UP BND       OF5_1       200.0000
 UP BND       OF5_2       200.0000
 UP BND       OF5_3       200.0000
 UP BND       OF6_1       200.0000
 UP BND       OF6_2       200.0000
 UP BND       OF6_3       200.0000
 FR BND       ADJ1      
 FR BND       ADJ2      
 FR BND       ADJ3      
 FX BND       Z            42.0000
ENDATA
