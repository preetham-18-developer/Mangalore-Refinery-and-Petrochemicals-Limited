#ifndef BHARATOPT_MPS_PARSER_HPP
#define BHARATOPT_MPS_PARSER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>

namespace bharatopt {

enum class MpsParseStatus {
    SUCCESS,
    FILE_NOT_FOUND,
    MISSING_ENDATA,
    UNKNOWN_SECTION,
    MALFORMED_ROW,
    MALFORMED_COLUMN,
    MALFORMED_BOUND,
    UNCLOSED_INTEGER_MARKER,
    INVALID_NUMERIC_VALUE
};

struct MpsParseResult {
    MpsParseStatus status{MpsParseStatus::FILE_NOT_FOUND};
    std::string error_message;
    std::string problem_name;
    LPModel model;
    
    size_t rows_parsed{0};
    size_t cols_parsed{0};
    size_t nonzeros_parsed{0};
    size_t integer_vars_count{0};
    size_t binary_vars_count{0};
    double parse_time_ms{0.0};
};

class MpsParser {
public:
    MpsParser() = default;

    // Parse MPS format file from filepath
    MpsParseResult parse_file(const std::string& filepath) const;

    // Parse MPS format string content directly
    MpsParseResult parse_string(const std::string& content, const std::string& problem_name_override = "") const;

private:
    std::vector<std::string> tokenize_line(const std::string& line) const;
};

} // namespace bharatopt

#endif // BHARATOPT_MPS_PARSER_HPP
