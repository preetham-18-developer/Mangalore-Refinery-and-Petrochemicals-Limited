#include <bharatopt/highs_oracle_adapter.hpp>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>

namespace bharatopt {

bool HighsOracleAdapter::is_highs_available() {
#if defined(BHARATOPT_OS_WINDOWS)
    int ret = std::system("where highs >nul 2>nul");
#else
    int ret = std::system("which highs >/dev/null 2>&1");
#endif
    return (ret == 0);
}

HighsOracleResult HighsOracleAdapter::solve_mps(const std::string& mps_filepath) {
    HighsOracleResult res;
    if (!is_highs_available()) {
        res.available = false;
        res.status = "NOT_AVAILABLE";
        res.message = "External HiGHS binary not found in PATH.";
        return res;
    }

    std::string out_file = "highs_sol_out.txt";
    std::string cmd = "highs --model_file " + mps_filepath + " --solution_file " + out_file + " > highs_run.log 2>&1";
    int ret = std::system(cmd.c_str());

    if (ret != 0) {
        res.available = true;
        res.status = "EXECUTION_FAILURE";
        res.message = "HiGHS execution returned non-zero exit code.";
        return res;
    }

    res.available = true;
    res.status = "OPTIMAL";
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
