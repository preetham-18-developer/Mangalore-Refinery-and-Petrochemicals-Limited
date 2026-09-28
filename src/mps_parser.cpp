#include <bharatopt/mps_parser.hpp>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cctype>

namespace bharatopt {

std::vector<std::string> MpsParser::tokenize_line(const std::string& line) const {
    std::vector<std::string> tokens;
    if (line.empty() || line[0] == '*') return tokens; // Comment line

    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

MpsParseResult MpsParser::parse_file(const std::string& filepath) const {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::ifstream file_up1("../" + filepath);
        if (file_up1.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file_up1)), std::istreambuf_iterator<char>());
            return parse_string(content);
        }
        std::ifstream file_up2("../../" + filepath);
        if (file_up2.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file_up2)), std::istreambuf_iterator<char>());
            return parse_string(content);
        }
        MpsParseResult res;
        res.status = MpsParseStatus::FILE_NOT_FOUND;
        res.error_message = "Could not open MPS file: " + filepath;
        return res;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return parse_string(content);
}

enum class MpsSection {
    NONE,
    NAME,
    OBJSENSE,
    ROWS,
    COLUMNS,
    RHS,
    RANGES,
    BOUNDS,
    ENDATA
};

MpsParseResult MpsParser::parse_string(const std::string& content, const std::string& problem_name_override) const {
    auto t_start = std::chrono::high_resolution_clock::now();
    MpsParseResult res;
    
    std::istringstream stream(content);
    std::string line;

    MpsSection section = MpsSection::NONE;
    std::string obj_row_name = "";
    bool explicit_objsense = false;
    ObjectiveSense mps_sense = ObjectiveSense::MINIMIZE;

    struct RowDef {
        std::string name;
        char type; // 'N', 'E', 'L', 'G'
    };
    std::unordered_map<std::string, RowDef> row_map;
    std::vector<std::string> row_order;

    struct ColumnCoeff {
        std::string row_name;
        real_t coeff;
    };
    std::unordered_map<std::string, std::vector<ColumnCoeff>> col_map;
    std::vector<std::string> col_order;
    std::unordered_map<std::string, VariableType> var_types;

    std::unordered_map<std::string, real_t> rhs_map;
    std::unordered_map<std::string, real_t> range_map;

    struct BoundDef {
        real_t lb{0.0};
        real_t ub{BHARATOPT_INFINITY};
        VariableType type{VariableType::CONTINUOUS};
        bool lb_specified{false};
        bool ub_specified{false};
        bool is_free{false};
    };
    std::unordered_map<std::string, BoundDef> bound_map;

    bool in_integer_section = false;
    bool found_endata = false;
    size_t line_number = 0;

    while (std::getline(stream, line)) {
        line_number++;
        if (line.empty() || line[0] == '*') continue;

        std::vector<std::string> tokens = tokenize_line(line);
        if (tokens.empty()) continue;

        std::string keyword = tokens[0];
        std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::toupper);

        bool is_header_line = (line[0] != ' ' && line[0] != '\t') || section == MpsSection::NONE;

        if (is_header_line) {
            if (keyword == "NAME") {
                section = MpsSection::NAME;
                if (tokens.size() > 1) res.problem_name = tokens[1];
                else res.problem_name = "unnamed_mps";
                if (!problem_name_override.empty()) res.problem_name = problem_name_override;
                continue;
            } else if (keyword == "OBJSENSE") {
                section = MpsSection::OBJSENSE;
                if (tokens.size() > 1) {
                    std::string sense_str = tokens[1];
                    std::transform(sense_str.begin(), sense_str.end(), sense_str.begin(), ::toupper);
                    if (sense_str == "MAX" || sense_str == "MAXIMIZE") {
                        mps_sense = ObjectiveSense::MAXIMIZE;
                        explicit_objsense = true;
                    } else if (sense_str == "MIN" || sense_str == "MINIMIZE") {
                        mps_sense = ObjectiveSense::MINIMIZE;
                        explicit_objsense = true;
                    }
                }
                continue;
            } else if (keyword == "ROWS") {
                section = MpsSection::ROWS;
                continue;
            } else if (keyword == "COLUMNS") {
                section = MpsSection::COLUMNS;
                continue;
            } else if (keyword == "RHS") {
                section = MpsSection::RHS;
                continue;
            } else if (keyword == "RANGES") {
                section = MpsSection::RANGES;
                continue;
            } else if (keyword == "BOUNDS") {
                section = MpsSection::BOUNDS;
                continue;
            } else if (keyword == "ENDATA") {
                section = MpsSection::ENDATA;
                found_endata = true;
                break;
            }
        }

        if (section == MpsSection::OBJSENSE) {
            std::string sense_str = tokens[0];
            std::transform(sense_str.begin(), sense_str.end(), sense_str.begin(), ::toupper);
            if (sense_str == "MAX" || sense_str == "MAXIMIZE") {
                mps_sense = ObjectiveSense::MAXIMIZE;
                explicit_objsense = true;
            } else if (sense_str == "MIN" || sense_str == "MINIMIZE") {
                mps_sense = ObjectiveSense::MINIMIZE;
                explicit_objsense = true;
            }
        } else if (section == MpsSection::ROWS) {
            if (tokens.size() < 2) {
                res.status = MpsParseStatus::MALFORMED_ROW;
                res.error_message = "Malformed ROWS line at line " + std::to_string(line_number);
                return res;
            }
            char rtype = std::toupper(tokens[0][0]);
            std::string rname = tokens[1];

            if (rtype != 'N' && rtype != 'E' && rtype != 'L' && rtype != 'G') {
                res.status = MpsParseStatus::MALFORMED_ROW;
                res.error_message = "Invalid row type '" + std::string(1, rtype) + "' at line " + std::to_string(line_number);
                return res;
            }

            if (rtype == 'N' && obj_row_name.empty()) {
                obj_row_name = rname;
            }

            row_map[rname] = {rname, rtype};
            row_order.push_back(rname);
        } else if (section == MpsSection::COLUMNS) {
            // Check for integer marker line: var_name 'MARKER' 'INTORG' / 'INTEND'
            bool is_marker = false;
            for (const auto& tok : tokens) {
                std::string utok = tok;
                std::transform(utok.begin(), utok.end(), utok.begin(), ::toupper);
                if (utok.find("'MARKER'") != std::string::npos || utok.find("MARKER") != std::string::npos) {
                    is_marker = true;
                    break;
                }
            }

            if (is_marker) {
                for (const auto& tok : tokens) {
                    std::string utok = tok;
                    std::transform(utok.begin(), utok.end(), utok.begin(), ::toupper);
                    if (utok.find("INTORG") != std::string::npos) {
                        in_integer_section = true;
                    } else if (utok.find("INTEND") != std::string::npos) {
                        in_integer_section = false;
                    }
                }
                continue;
            }

            if (tokens.size() < 3) {
                res.status = MpsParseStatus::MALFORMED_COLUMN;
                res.error_message = "Malformed COLUMNS line at line " + std::to_string(line_number);
                return res;
            }

            std::string col_name = tokens[0];
            if (col_map.find(col_name) == col_map.end()) {
                col_order.push_back(col_name);
                col_map[col_name] = {};
                var_types[col_name] = in_integer_section ? VariableType::INTEGER : VariableType::CONTINUOUS;
            }

            for (size_t k = 1; k < tokens.size(); k += 2) {
                if (k + 1 >= tokens.size()) break;
                std::string rname = tokens[k];
                try {
                    real_t val = std::stod(tokens[k + 1]);
                    col_map[col_name].push_back({rname, val});
                } catch (...) {
                    res.status = MpsParseStatus::INVALID_NUMERIC_VALUE;
                    res.error_message = "Invalid coefficient '" + tokens[k + 1] + "' at line " + std::to_string(line_number);
                    return res;
                }
            }
        } else if (section == MpsSection::RHS) {
            size_t start_idx = 0;
            if (row_map.find(tokens[0]) == row_map.end() && tokens.size() > 1) {
                start_idx = 1;
            }
            for (size_t k = start_idx; k < tokens.size(); k += 2) {
                if (k + 1 >= tokens.size()) break;
                std::string rname = tokens[k];
                try {
                    real_t val = std::stod(tokens[k + 1]);
                    rhs_map[rname] = val;
                } catch (...) {
                    res.status = MpsParseStatus::INVALID_NUMERIC_VALUE;
                    res.error_message = "Invalid RHS value at line " + std::to_string(line_number);
                    return res;
                }
            }
        } else if (section == MpsSection::RANGES) {
            size_t start_idx = (tokens.size() % 2 == 1) ? 1 : 0;
            for (size_t k = start_idx; k < tokens.size(); k += 2) {
                if (k + 1 >= tokens.size()) break;
                std::string rname = tokens[k];
                try {
                    real_t val = std::stod(tokens[k + 1]);
                    range_map[rname] = val;
                } catch (...) {
                    res.status = MpsParseStatus::INVALID_NUMERIC_VALUE;
                    res.error_message = "Invalid RANGES value at line " + std::to_string(line_number);
                    return res;
                }
            }
        } else if (section == MpsSection::BOUNDS) {
            if (tokens.size() < 3) {
                res.status = MpsParseStatus::MALFORMED_BOUND;
                res.error_message = "Malformed BOUNDS line at line " + std::to_string(line_number);
                return res;
            }

            std::string btype = tokens[0];
            std::transform(btype.begin(), btype.end(), btype.begin(), ::toupper);

            size_t var_idx = 2;
            size_t val_idx = 3;
            if (tokens.size() == 3) {
                var_idx = 2;
                val_idx = 2; // Default 0
            }

            std::string vname = tokens[var_idx];
            auto& bdef = bound_map[vname];

            real_t val = 0.0;
            if (val_idx < tokens.size()) {
                try {
                    val = std::stod(tokens[val_idx]);
                } catch (...) {
                    val = 0.0;
                }
            }

            if (btype == "LO") {
                bdef.lb = val;
                bdef.lb_specified = true;
            } else if (btype == "UP") {
                bdef.ub = val;
                bdef.ub_specified = true;
            } else if (btype == "FX") {
                bdef.lb = val;
                bdef.ub = val;
                bdef.lb_specified = true;
                bdef.ub_specified = true;
            } else if (btype == "FR") {
                bdef.is_free = true;
                bdef.lb = -BHARATOPT_INFINITY;
                bdef.ub = BHARATOPT_INFINITY;
            } else if (btype == "MI") {
                bdef.lb = -BHARATOPT_INFINITY;
            } else if (btype == "PL") {
                bdef.ub = BHARATOPT_INFINITY;
            } else if (btype == "BV") {
                bdef.lb = 0.0;
                bdef.ub = 1.0;
                bdef.type = VariableType::BINARY;
                bdef.lb_specified = true;
                bdef.ub_specified = true;
            } else if (btype == "LI") {
                bdef.lb = val;
                bdef.type = VariableType::INTEGER;
                bdef.lb_specified = true;
            } else if (btype == "UI") {
                bdef.ub = val;
                bdef.type = VariableType::INTEGER;
                bdef.ub_specified = true;
            } else {
                res.status = MpsParseStatus::MALFORMED_BOUND;
                res.error_message = "Unknown bound type '" + btype + "' at line " + std::to_string(line_number);
                return res;
            }
        }
    }

    if (!found_endata) {
        res.status = MpsParseStatus::MISSING_ENDATA;
        res.error_message = "MPS file missing ENDATA card";
        return res;
    }

    if (in_integer_section) {
        res.status = MpsParseStatus::UNCLOSED_INTEGER_MARKER;
        res.error_message = "MPS file contains unclosed integer marker section (missing INTEND)";
        return res;
    }

    // --- CONSTRUCT LPMODEL FROM PARSED MPS DATA ---
    LPModel model(res.problem_name.empty() ? "mps_model" : res.problem_name);
    if (explicit_objsense) {
        model.set_sense(mps_sense);
    }
    auto it_obj_rhs = rhs_map.find(obj_row_name);
    if (it_obj_rhs != rhs_map.end()) {
        model.set_obj_offset(-it_obj_rhs->second);
    }

    std::unordered_map<std::string, index_t> var_idx_map;
    size_t int_cnt = 0, bin_cnt = 0;

    for (const auto& vname : col_order) {
        VariableType vtype = var_types[vname];
        real_t lb = 0.0;
        real_t ub = BHARATOPT_INFINITY;

        auto it_b = bound_map.find(vname);
        if (it_b != bound_map.end()) {
            const auto& bdef = it_b->second;
            if (bdef.is_free || (bdef.lb <= -BHARATOPT_INFINITY && bdef.ub >= BHARATOPT_INFINITY)) {
                lb = -BHARATOPT_INFINITY;
                ub = BHARATOPT_INFINITY;
            } else {
                lb = bdef.lb;
                ub = bdef.ub;
            }
            if (bdef.type == VariableType::BINARY) vtype = VariableType::BINARY;
            else if (bdef.type == VariableType::INTEGER && vtype != VariableType::BINARY) vtype = VariableType::INTEGER;
        }

        if (vtype == VariableType::BINARY) bin_cnt++;
        else if (vtype == VariableType::INTEGER) int_cnt++;

        // Find objective coefficient
        real_t obj_c = 0.0;
        for (const auto& coeff : col_map[vname]) {
            if (coeff.row_name == obj_row_name) {
                obj_c = coeff.coeff;
                break;
            }
        }

        index_t vidx = model.add_variable(vname, lb, ub, obj_c, vtype);
        var_idx_map[vname] = vidx;
    }

    size_t nonzeros_cnt = 0;
    std::unordered_map<std::string, index_t> cons_idx_map;

    for (const auto& rname : row_order) {
        if (rname == obj_row_name) continue;

        const auto& rdef = row_map[rname];
        real_t rhs_val = 0.0;
        auto it_rhs = rhs_map.find(rname);
        if (it_rhs != rhs_map.end()) rhs_val = it_rhs->second;

        ConstraintSense csense = ConstraintSense::LESS_EQUAL;
        if (rdef.type == 'E') csense = ConstraintSense::EQUAL;
        else if (rdef.type == 'G') csense = ConstraintSense::GREATER_EQUAL;
        else if (rdef.type == 'L') csense = ConstraintSense::LESS_EQUAL;

        real_t range_up = 0.0;
        auto it_rng = range_map.find(rname);
        if (it_rng != range_map.end()) {
            csense = ConstraintSense::RANGED;
            real_t rval = it_rng->second;
            if (rdef.type == 'E') {
                if (rval > 0) {
                    range_up = rhs_val + rval;
                } else {
                    range_up = rhs_val;
                    rhs_val = rhs_val + rval;
                }
            } else if (rdef.type == 'L') {
                range_up = rhs_val;
                rhs_val = rhs_val - std::abs(rval);
            } else if (rdef.type == 'G') {
                range_up = rhs_val + std::abs(rval);
            }
        }

        index_t cidx = model.add_constraint(rname, csense, rhs_val, range_up);
        cons_idx_map[rname] = cidx;
    }

    for (const auto& vname : col_order) {
        index_t vidx = var_idx_map[vname];
        for (const auto& coeff : col_map[vname]) {
            if (coeff.row_name == obj_row_name) continue;
            auto it_c = cons_idx_map.find(coeff.row_name);
            if (it_c != cons_idx_map.end()) {
                model.set_coeff(it_c->second, vidx, coeff.coeff);
                nonzeros_cnt++;
            }
        }
    }

    auto t_end = std::chrono::high_resolution_clock::now();

    res.status = MpsParseStatus::SUCCESS;
    res.model = model;
    res.rows_parsed = model.num_constraints();
    res.cols_parsed = model.num_variables();
    res.nonzeros_parsed = nonzeros_cnt;
    res.integer_vars_count = int_cnt;
    res.binary_vars_count = bin_cnt;
    res.parse_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    return res;
}

} // namespace bharatopt
