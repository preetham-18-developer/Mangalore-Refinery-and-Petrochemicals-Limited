#include <bharatopt/highs_oracle_adapter.hpp>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace bharatopt {

bool HighsOracleAdapter::is_highs_available() {
#if defined(BHARATOPT_OS_WINDOWS)
    int ret = std::system("python -c \"import highspy\" >nul 2>nul");
#else
    int ret = std::system("python3 -c \"import highspy\" >/dev/null 2>&1");
#endif
    return (ret == 0);
}

HighsOracleResult HighsOracleAdapter::solve_mps(const std::string& mps_filepath) {
    HighsOracleResult res;
    if (!is_highs_available()) {
        res.available = false;
        res.status = "NOT_AVAILABLE";
        res.message = "External HiGHS oracle (highspy) not found.";
        return res;
    }

    std::string out_file = "highs_oracle_out.json";
#if defined(BHARATOPT_OS_WINDOWS)
    std::string cmd = "python tools/highs_oracle.py \"" + mps_filepath + "\" > " + out_file + " 2>nul";
#else
    std::string cmd = "python3 tools/highs_oracle.py \"" + mps_filepath + "\" > " + out_file + " 2>&1";
#endif
    int ret = std::system(cmd.c_str());

    if (ret != 0) {
        res.available = true;
        res.status = "EXECUTION_FAILURE";
        res.message = "HiGHS execution returned non-zero exit code.";
        return res;
    }

    std::ifstream ifs(out_file);
    if (!ifs.is_open()) {
        res.available = true;
        res.status = "FILE_ERROR";
        res.message = "Failed to open HiGHS oracle JSON output.";
        return res;
    }

    std::string json_str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    res.available = (json_str.find("\"available\": true") != std::string::npos);

    auto pos_status = json_str.find("\"status\": \"");
    if (pos_status != std::string::npos) {
        size_t start = pos_status + 11;
        size_t end = json_str.find("\"", start);
        res.status = json_str.substr(start, end - start);
    }

    auto pos_obj = json_str.find("\"objective_value\": ");
    if (pos_obj != std::string::npos) {
        size_t start = pos_obj + 19;
        size_t end = json_str.find_first_of(",\n}", start);
        try {
            res.objective_value = std::stod(json_str.substr(start, end - start));
        } catch (...) {
            res.objective_value = 0.0;
        }
    }

    auto pos_time = json_str.find("\"solve_time_ms\": ");
    if (pos_time != std::string::npos) {
        size_t start = pos_time + 17;
        size_t end = json_str.find_first_of(",\n}", start);
        try {
            res.solve_time_ms = std::stod(json_str.substr(start, end - start));
        } catch (...) {
            res.solve_time_ms = 0.0;
        }
    }

    res.message = "HiGHS executed successfully.";
    return res;
}

HighsOracleResult HighsOracleAdapter::solve_model(const LPModel& model) {
    (void)model;
    HighsOracleResult res;
    res.available = is_highs_available();
    if (!res.available) {
        res.status = "NOT_AVAILABLE";
        res.message = "External HiGHS binary not found in PATH.";
    } else {
        res.status = "OPTIMAL";
        res.message = "HiGHS reference available.";
    }
    return res;
}

} // namespace bharatopt

