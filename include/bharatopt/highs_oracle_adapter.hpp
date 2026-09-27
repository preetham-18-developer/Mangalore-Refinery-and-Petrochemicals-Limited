#ifndef BHARATOPT_HIGHS_ORACLE_ADAPTER_HPP
#define BHARATOPT_HIGHS_ORACLE_ADAPTER_HPP

#include <string>
#include <vector>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>

namespace bharatopt {

struct HighsOracleResult {
    bool available{false};
    std::string status{"NOT_AVAILABLE"};
    real_t objective_value{0.0};
    double solve_time_ms{0.0};
    size_t iterations{0};
    size_t node_count{0};
    std::vector<real_t> primal_solution;
    std::string message;
};

/**
 * Isolated External HiGHS Benchmark Oracle Adapter.
 * HiGHS is executed strictly as an external reference oracle for benchmark comparison.
 * HiGHS is NOT linked into BharatOpt solver libraries.
 */
class HighsOracleAdapter {
public:
    HighsOracleAdapter() = default;

    // Check if external HiGHS CLI executable is available on host system
    static bool is_highs_available();

    // Run external HiGHS oracle on an MPS file instance
    static HighsOracleResult solve_mps(const std::string& mps_filepath);

    // Run external HiGHS oracle on an LPModel instance
    static HighsOracleResult solve_model(const LPModel& model);
};

} // namespace bharatopt

#endif // BHARATOPT_HIGHS_ORACLE_ADAPTER_HPP
