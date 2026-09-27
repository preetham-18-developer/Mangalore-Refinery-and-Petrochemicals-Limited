#ifndef BHARATOPT_LP_MODEL_HPP
#define BHARATOPT_LP_MODEL_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <stdexcept>
#include <cmath>
#include <bharatopt/config.hpp>

namespace bharatopt {

enum class ObjectiveSense {
    MINIMIZE,
    MAXIMIZE
};

enum class ConstraintSense {
    LESS_EQUAL,     // Ax <= b
    EQUAL,          // Ax = b
    GREATER_EQUAL,  // Ax >= b
    RANGED          // l <= Ax <= u
};

enum class VariableType {
    CONTINUOUS,
    INTEGER,
    BINARY
};

struct Variable {
    std::string name;
    real_t lower_bound{0.0};
    real_t upper_bound{BHARATOPT_INFINITY};
    real_t obj_coeff{0.0};
    VariableType type{VariableType::CONTINUOUS};
    index_t index{-1};

    bool is_free() const {
        return (lower_bound <= -BHARATOPT_INFINITY || std::isinf(lower_bound)) &&
               (upper_bound >= BHARATOPT_INFINITY || std::isinf(upper_bound));
    }

    bool is_fixed() const {
        return lower_bound == upper_bound;
    }
};

struct Constraint {
    std::string name;
    ConstraintSense sense{ConstraintSense::LESS_EQUAL};
    real_t rhs{0.0};
    real_t range_upper{0.0}; // Used when sense == RANGED
    index_t index{-1};
    // Sparse row representation: vector of (var_index, coefficient)
    std::vector<std::pair<index_t, real_t>> terms;
};

class LPModel {
public:
    explicit LPModel(std::string name = "unnamed_model");

    // Model metadata
    const std::string& name() const { return name_; }
    void set_name(const std::string& name) { name_ = name; }

    ObjectiveSense sense() const { return sense_; }
    void set_sense(ObjectiveSense sense) { sense_ = sense; }

    real_t obj_offset() const { return obj_offset_; }
    void set_obj_offset(real_t offset) { obj_offset_ = offset; }

    // Variable management
    index_t add_variable(const std::string& name,
                         real_t lower_bound = 0.0,
                         real_t upper_bound = BHARATOPT_INFINITY,
                         real_t obj_coeff = 0.0,
                         VariableType type = VariableType::CONTINUOUS);

    size_t num_variables() const { return variables_.size(); }
    const Variable& get_variable(index_t index) const;
    Variable& get_variable(index_t index);
    const Variable& get_variable(const std::string& name) const;
    bool has_variable(const std::string& name) const;

    // Constraint management
    index_t add_constraint(const std::string& name,
                           ConstraintSense sense,
                           real_t rhs,
                           real_t range_upper = 0.0);

    index_t add_constraint(const std::string& name,
                           const std::vector<std::pair<index_t, real_t>>& terms,
                           ConstraintSense sense,
                           real_t rhs,
                           real_t range_upper = 0.0);

    size_t num_constraints() const { return constraints_.size(); }
    const Constraint& get_constraint(index_t index) const;
    Constraint& get_constraint(index_t index);
    const Constraint& get_constraint(const std::string& name) const;
    bool has_constraint(const std::string& name) const;

    // Matrix coefficient access
    void set_coeff(index_t constraint_idx, index_t var_idx, real_t value);
    real_t get_coeff(index_t constraint_idx, index_t var_idx) const;

    // Statistics
    size_t num_nonzeros() const;
    void clear();

    // Accessors for raw collections
    const std::vector<Variable>& variables() const { return variables_; }
    const std::vector<Constraint>& constraints() const { return constraints_; }

private:
    std::string name_;
    ObjectiveSense sense_{ObjectiveSense::MINIMIZE};
    real_t obj_offset_{0.0};

    std::vector<Variable> variables_;
    std::vector<Constraint> constraints_;

    std::unordered_map<std::string, index_t> var_name_to_idx_;
    std::unordered_map<std::string, index_t> cons_name_to_idx_;
};

} // namespace bharatopt

#endif // BHARATOPT_LP_MODEL_HPP
