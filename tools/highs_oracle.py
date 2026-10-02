import sys
import os
import json
import time

def run_highs_oracle(mps_path, force_lp=False):
    try:
        import highspy
    except ImportError:
        return {
            "available": False,
            "status": "NOT_AVAILABLE",
            "message": "highspy module not found in Python environment.",
            "objective_value": 0.0,
            "solve_time_ms": 0.0,
            "solution": {}
        }

    if not os.path.exists(mps_path):
        return {
            "available": True,
            "status": "FILE_NOT_FOUND",
            "message": f"File not found: {mps_path}",
            "objective_value": 0.0,
            "solve_time_ms": 0.0,
            "solution": {}
        }

    h = highspy.Highs()
    h.setOptionValue("output_flag", False)
    
    status = h.readModel(mps_path)
    if status != highspy.HighsStatus.kOk:
        return {
            "available": True,
            "status": "PARSE_ERROR",
            "message": f"HiGHS failed to parse MPS file: {mps_path}",
            "objective_value": 0.0,
            "solve_time_ms": 0.0,
            "solution": {}
        }

    if force_lp:
        lp = h.getLp()
        # Change integer/binary variables to continuous
        num_col = lp.num_col_
        for col in range(num_col):
            h.changeColIntegrality(col, highspy.HighsVarType.kContinuous)

    t0 = time.perf_counter()
    h.run()
    t1 = time.perf_counter()
    solve_time_ms = (t1 - t0) * 1000.0

    model_status = h.getModelStatus()
    status_str = str(model_status).replace("HighsModelStatus.k", "").upper()
    if "OPTIMAL" in status_str:
        status_str = "OPTIMAL"
    elif "INFEASIBLE" in status_str:
        status_str = "INFEASIBLE"
    elif "UNBOUNDED" in status_str:
        status_str = "UNBOUNDED"

    info = h.getInfo()
    obj_val = h.getObjectiveValue() if status_str == "OPTIMAL" else 0.0

    sol = h.getSolution()
    var_vals = {}
    if status_str == "OPTIMAL" and sol.col_value:
        lp = h.getLp()
        # If column names exist
        names = lp.col_names_ if hasattr(lp, "col_names_") and len(lp.col_names_) == len(sol.col_value) else []
        for i, val in enumerate(sol.col_value):
            name = names[i] if i < len(names) else f"x{i}"
            var_vals[name] = float(val)

    return {
        "available": True,
        "status": status_str,
        "objective_value": float(obj_val),
        "solve_time_ms": solve_time_ms,
        "iterations": getattr(info, 'simplex_iteration_count', 0),
        "node_count": getattr(info, 'ipm_iteration_count', 0),
        "solution": var_vals
    }

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(json.dumps({"available": False, "status": "INVALID_USAGE", "message": "Usage: python highs_oracle.py <file.mps> [--relaxation]"}))
        sys.exit(1)

    mps_file = sys.argv[1]
    force_lp = "--relaxation" in sys.argv or "-lp" in sys.argv
    res = run_highs_oracle(mps_file, force_lp=force_lp)
    print(json.dumps(res, indent=2))
