import os
import json
import urllib.request
import gzip
import subprocess
import highspy
import numpy as np

def download_or_get(name, url):
    cache_dir = "benchmarks/corpus_raw"
    os.makedirs(cache_dir, exist_ok=True)
    cache_path = os.path.join(cache_dir, f"{name}.mps")
    
    if os.path.exists(cache_path):
        with open(cache_path, 'r', encoding='utf-8') as f:
            return f.read()
            
    data = urllib.request.urlopen(url).read()
    if url.endswith('.gz'):
        data = gzip.decompress(data)
    content = data.decode('utf-8')
    with open(cache_path, 'w', encoding='utf-8') as f:
        f.write(content)
    return content

def convert_to_fixed(free_str):
    fixed_lines = []
    lines = free_str.splitlines()
    section = None
    for line in lines:
        sline = line.strip()
        if not sline:
            continue
        first_word = sline.split()[0]
        if first_word in ['NAME', 'ROWS', 'COLUMNS', 'RHS', 'RANGES', 'BOUNDS', 'ENDATA']:
            fixed_lines.append(line)
            section = first_word
            continue
        
        if section == 'ROWS':
            toks = sline.split()
            if len(toks) >= 2:
                fixed_lines.append(f" {toks[0]:<2} {toks[1]:<8}")
            else:
                fixed_lines.append(line)
        elif section in ['COLUMNS', 'RHS', 'RANGES']:
            toks = sline.split()
            if len(toks) == 3:
                try:
                    val = float(toks[2])
                    fixed_lines.append(f"    {toks[0]:<8}  {toks[1]:<8}  {val:>12.5f}")
                except ValueError:
                    fixed_lines.append(line)
            elif len(toks) == 5:
                try:
                    v1 = float(toks[2])
                    v2 = float(toks[4])
                    fixed_lines.append(f"    {toks[0]:<8}  {toks[1]:<8}  {v1:>12.5f}   {toks[3]:<8}  {v2:>12.5f}")
                except ValueError:
                    fixed_lines.append(line)
            else:
                fixed_lines.append(line)
        elif section == 'BOUNDS':
            toks = sline.split()
            if len(toks) == 4:
                try:
                    val = float(toks[3])
                    fixed_lines.append(f" {toks[0]:<2} {toks[1]:<8}  {toks[2]:<8}  {val:>12.5f}")
                except ValueError:
                    fixed_lines.append(line)
            elif len(toks) == 3:
                fixed_lines.append(f" {toks[0]:<2} {toks[1]:<8}  {toks[2]:<8}")
            else:
                fixed_lines.append(line)
        else:
            fixed_lines.append(line)
    return '\n'.join(fixed_lines) + '\n'

def prepare_corpus():
    os.makedirs('benchmarks/conformance', exist_ok=True)
    
    # 1. NETLIB & MIPLIB Standard URLs
    urls = {
        'afiro': ('https://raw.githubusercontent.com/coin-or-tools/Data-Sample/master/afiro.mps', 27, 32, 83),
        'adlittle': ('https://raw.githubusercontent.com/coin-or-tools/Data-Netlib/master/adlittle.mps.gz', 56, 97, 383),
        'blend': ('https://raw.githubusercontent.com/coin-or-tools/Data-Netlib/master/blend.mps.gz', 74, 83, 491),
        'sc50a': ('https://raw.githubusercontent.com/coin-or-tools/Data-Netlib/master/sc50a.mps.gz', 50, 48, 130),
        'kb2': ('https://raw.githubusercontent.com/coin-or-tools/Data-Netlib/master/kb2.mps.gz', 43, 41, 286),
        'share2b': ('https://raw.githubusercontent.com/coin-or-tools/Data-Netlib/master/share2b.mps.gz', 96, 79, 694),
        'p0033': ('https://raw.githubusercontent.com/coin-or/CyLP/master/cylp/input/p0033.mps', 16, 33, 98),
    }

    models_info = {}
    for name, (url, exp_rows, exp_cols, exp_nnz) in urls.items():
        free_content = download_or_get(name, url)
        fixed_content = convert_to_fixed(free_content)
        
        free_path = f"benchmarks/conformance/{name}.free.mps"
        fixed_path = f"benchmarks/conformance/{name}.fixed.mps"
        
        with open(free_path, 'w', encoding='utf-8') as f: f.write(free_content)
        with open(fixed_path, 'w', encoding='utf-8') as f: f.write(fixed_content)
        
        models_info[name] = (exp_rows, exp_cols, exp_nnz)

    with open('benchmarks/miplib/blend2.mps', 'r', encoding='utf-8') as f:
        b2_content = f.read()
    with open('benchmarks/conformance/blend2.free.mps', 'w', encoding='utf-8') as f: f.write(b2_content)
    with open('benchmarks/conformance/blend2.fixed.mps', 'w', encoding='utf-8') as f: f.write(convert_to_fixed(b2_content))
    models_info['blend2'] = (3, 2, 4)

    # 2. Synthetic Edge Case Models
    edge_cases = {}
    
    # Edge 1: Objective Row RHS
    edge_cases['edge_obj_rhs'] = """NAME          EDGE_OBJ_RHS
ROWS
 N  COST
 L  C1
COLUMNS
    X1        COST           3.00000   C1           1.00000
RHS
    RHS1      COST          50.00000   C1          10.00000
ENDATA
"""

    # Edge 2: Fixed variable with cost
    edge_cases['edge_fx_cost'] = """NAME          EDGE_FX_COST
ROWS
 N  COST
 L  C1
COLUMNS
    X1        COST           3.00000   C1           1.00000
    X2        COST           5.00000   C1           1.00000
RHS
    RHS1      C1            20.00000
BOUNDS
 FX BND       X1            10.00000
 UP BND       X2            50.00000
ENDATA
"""

    # Edge 3: Negative Range on E row
    edge_cases['edge_neg_range_e'] = """NAME          EDGE_NEG_RANGE_E
ROWS
 N  COST
 E  C1
COLUMNS
    X1        COST           2.00000   C1           1.00000
RHS
    RHS1      C1            100.00000
RANGES
    RNG1      C1            -20.00000
BOUNDS
 UP BND       X1            200.00000
ENDATA
"""

    # Edge 4: MI, PL, BV bounds
    edge_cases['edge_mi_pl_bv'] = """NAME          EDGE_MI_PL_BV
ROWS
 N  COST
 L  C1
COLUMNS
    MARK0000  'MARKER'                 'INTORG'
    X1        COST           1.00000   C1           1.00000
    X2        COST           2.00000   C1           1.00000
    MARK0001  'MARKER'                 'INTEND'
RHS
    RHS1      C1             5.00000
BOUNDS
 MI BND       X1
 PL BND       X1
 BV BND       X2
ENDATA
"""

    # Edge 5: Infeasible LP
    edge_cases['edge_infeasible'] = """NAME          EDGE_INFEASIBLE
ROWS
 N  COST
 L  C1
 G  C2
COLUMNS
    X1        COST           1.00000   C1           1.00000
    X1        C2             1.00000
RHS
    RHS1      C1             5.00000   C2          10.00000
ENDATA
"""

    # Edge 6: Unbounded LP
    edge_cases['edge_unbounded'] = """NAME          EDGE_UNBOUNDED
ROWS
 N  COST
 L  C1
COLUMNS
    X1        COST          -5.00000   C1          -1.00000
RHS
    RHS1      C1             0.00000
BOUNDS
 FR BND       X1
ENDATA
"""

    # Edge 7: Degenerate LP
    edge_cases['edge_degenerate'] = """NAME          EDGE_DEGENERATE
ROWS
 N  COST
 L  C1
 L  C2
COLUMNS
    X1        COST           1.00000   C1           1.00000
    X2        COST           1.00000   C2           1.00000
RHS
    RHS1      C1             0.00000   C2             0.00000
ENDATA
"""

    for ename, econtent in edge_cases.items():
        efree = f"benchmarks/conformance/{ename}.free.mps"
        efixed = f"benchmarks/conformance/{ename}.fixed.mps"
        with open(efree, 'w', encoding='utf-8') as f: f.write(econtent)
        with open(efixed, 'w', encoding='utf-8') as f: f.write(convert_to_fixed(econtent))

    return models_info

def solve_with_highs(filepath, is_milp=False):
    import time
    t0 = time.perf_counter()
    h = highspy.Highs()
    h.setOptionValue("output_flag", False)
    status = h.readModel(filepath)
    if status != highspy.HighsStatus.kOk:
        return "PARSE_ERROR", 0.0, 0.0
    
    if not is_milp:
        # Relax integrality for LP evaluation mode
        lp = h.getLp()
        if len(lp.integrality_) > 0:
            for i in range(len(lp.integrality_)):
                lp.integrality_[i] = highspy.HighsVarType.kContinuous
            h.passModel(lp)

    h.run()
    t1 = time.perf_counter()
    info = h.getInfo()
    highs_status_str = h.modelStatusToString(h.getModelStatus())

    if highs_status_str == "Optimal":
        st = "OPTIMAL"
    elif highs_status_str in ["Infeasible", "Primal infeasible"]:
        st = "INFEASIBLE"
    elif "UNBOUNDED" in highs_status_str.upper():
        st = "UNBOUNDED"
    else:
        st = highs_status_str.upper()

    obj = info.objective_function_value if st == "OPTIMAL" else 0.0
    return st, obj, (t1 - t0) * 1000.0

def solve_with_bharatopt(filepath, mode="AUTO"):
    cmd = ["build/conformance_runner.exe", filepath, mode]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        return "EXEC_FAIL", 0.0, 0.0
    try:
        data = json.loads(proc.stdout)
        return data["status"], data["objective"], data["solve_time_ms"]
    except Exception:
        return "JSON_FAIL", 0.0, 0.0

def main():
    models_info = prepare_corpus()

    # List of test instances: (filename, mode, expected_rows, expected_cols, expected_nnz)
    test_suite = [
        # NETLIB Real LPs
        ("afiro.free.mps", "LP", 27, 32, 83),
        ("afiro.fixed.mps", "LP", 27, 32, 83),
        ("adlittle.free.mps", "LP", 56, 97, 383),
        ("adlittle.fixed.mps", "LP", 56, 97, 383),
        ("blend.free.mps", "LP", 74, 83, 491),
        ("blend.fixed.mps", "LP", 74, 83, 491),
        ("sc50a.free.mps", "LP", 50, 48, 130),
        ("sc50a.fixed.mps", "LP", 50, 48, 130),
        ("kb2.free.mps", "LP", 43, 41, 286),
        ("kb2.fixed.mps", "LP", 43, 41, 286),
        ("share2b.free.mps", "LP", 96, 79, 694),
        ("share2b.fixed.mps", "LP", 96, 79, 694),
        
        # MIPLIB Easy Models
        ("p0033.free.mps", "MILP", 16, 33, 98),
        ("p0033.fixed.mps", "MILP", 16, 33, 98),
        ("blend2.free.mps", "MILP", 3, 2, 4),
        ("blend2.fixed.mps", "MILP", 3, 2, 4),
        
        # Edge Cases
        ("edge_obj_rhs.free.mps", "LP", 1, 1, 1),
        ("edge_obj_rhs.fixed.mps", "LP", 1, 1, 1),
        ("edge_fx_cost.free.mps", "LP", 1, 2, 2),
        ("edge_fx_cost.fixed.mps", "LP", 1, 2, 2),
        ("edge_neg_range_e.free.mps", "LP", 1, 1, 1),
        ("edge_neg_range_e.fixed.mps", "LP", 1, 1, 1),
        ("edge_mi_pl_bv.free.mps", "LP", 1, 2, 2),
        ("edge_mi_pl_bv.fixed.mps", "LP", 1, 2, 2),
        ("edge_infeasible.free.mps", "LP", 2, 1, 2),
        ("edge_infeasible.fixed.mps", "LP", 2, 1, 2),
        ("edge_unbounded.free.mps", "LP", 1, 1, 1),
        ("edge_unbounded.fixed.mps", "LP", 1, 1, 1),
        ("edge_degenerate.free.mps", "LP", 2, 2, 2),
        ("edge_degenerate.fixed.mps", "LP", 2, 2, 2),
    ]

    print("| Model | Oracle Status / Obj | BHARATOPT Status / Obj | Time (ms) | Result |")
    print("|---|---|---|---|---|")

    all_pass = True

    for fname, mode, exp_r, exp_c, exp_nz in test_suite:
        fpath = os.path.join("benchmarks/conformance", fname)
        is_milp = (mode == "MILP")

        h_status, h_obj, h_time = solve_with_highs(fpath, is_milp)
        b_status, b_obj, b_time = solve_with_bharatopt(fpath, mode)

        # Objective relative tolerance check (1e-6)
        rel_diff = 0.0
        if h_status == "OPTIMAL" and b_status == "OPTIMAL":
            denom = max(1.0, abs(h_obj))
            rel_diff = abs(b_obj - h_obj) / denom

        status_match = (h_status == b_status)
        obj_match = (h_status != "OPTIMAL") or (rel_diff <= 1e-6)

        passed = status_match and obj_match
        if not passed:
            all_pass = False

        res_str = "PASS" if passed else "FAIL"
        h_str = f"{h_status} / {h_obj:.6f}" if h_status == "OPTIMAL" else f"{h_status}"
        b_str = f"{b_status} / {b_obj:.6f}" if b_status == "OPTIMAL" else f"{b_status}"

        print(f"| {fname} | {h_str} | {b_str} | {b_time:.2f} | {res_str} |")

    # LP vs MILP objective check
    milp_models = ["p0033.free.mps", "blend2.free.mps", "edge_mi_pl_bv.free.mps"]
    for mname in milp_models:
        fpath = os.path.join("benchmarks/conformance", mname)
        lp_st, lp_obj, _ = solve_with_bharatopt(fpath, "LP")
        milp_st, milp_obj, _ = solve_with_bharatopt(fpath, "MILP")
        ineq_ok = (lp_obj <= milp_obj + 1e-6)
        if not ineq_ok:
            all_pass = False

    if not all_pass:
        exit(1)

if __name__ == "__main__":
    main()
