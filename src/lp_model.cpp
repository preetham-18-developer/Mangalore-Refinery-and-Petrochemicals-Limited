#include <bharatopt/lp_model.hpp>
#include <algorithm>

namespace bharatopt {

LPModel::LPModel(std::string name)
    : name_(std::move(name)) {}

index_t LPModel::add_variable(const std::string& name,
                               real_t lower_bound,
                               real_t upper_bound,
                               real_t obj_coeff,
                               VariableType type) {
    if (var_name_to_idx_.find(name) != var_name_to_idx_.end()) {
        throw std::invalid_argument("Variable with name '" + name + "' already exists in LPModel.");
    }

    index_t new_idx = static_cast<index_t>(variables_.size());
    Variable var;
    var.name = name;
    var.lower_bound = lower_bound;
    var.upper_bound = upper_bound;
    var.obj_coeff = obj_coeff;
    var.type = type;
    var.index = new_idx;

    variables_.push_back(var);
    var_name_to_idx_[name] = new_idx;
    return new_idx;
}

const Variable& LPModel::get_variable(index_t index) const {
    if (index < 0 || static_cast<size_t>(index) >= variables_.size()) {
        throw std::out_of_range("Variable index out of range: " + std::to_string(index));
    }
    return variables_[static_cast<size_t>(index)];
}

Variable& LPModel::get_variable(index_t index) {
    if (index < 0 || static_cast<size_t>(index) >= variables_.size()) {
        throw std::out_of_range("Variable index out of range: " + std::to_string(index));
    }
    return variables_[static_cast<size_t>(index)];
}

const Variable& LPModel::get_variable(const std::string& name) const {
    auto it = var_name_to_idx_.find(name);
    if (it == var_name_to_idx_.end()) {
        throw std::invalid_argument("Variable name not found: " + name);
    }
    return variables_[static_cast<size_t>(it->second)];
}

bool LPModel::has_variable(const std::string& name) const {
    return var_name_to_idx_.find(name) != var_name_to_idx_.end();
}

index_t LPModel::add_constraint(const std::string& name,
                                  ConstraintSense sense,
                                  real_t rhs,
                                  real_t range_upper) {
    return add_constraint(name, {}, sense, rhs, range_upper);
}

index_t LPModel::add_constraint(const std::string& name,
                                  const std::vector<std::pair<index_t, real_t>>& terms,
                                  ConstraintSense sense,
                                  real_t rhs,
                                  real_t range_upper) {
    if (cons_name_to_idx_.find(name) != cons_name_to_idx_.end()) {
        throw std::invalid_argument("Constraint with name '" + name + "' already exists in LPModel.");
    }

    // Validate variable indices in terms
    for (const auto& term : terms) {
        if (term.first < 0 || static_cast<size_t>(term.first) >= variables_.size()) {
            throw std::out_of_range("Invalid variable index in constraint terms: " + std::to_string(term.first));
        }
    }

    index_t new_idx = static_cast<index_t>(constraints_.size());
    Constraint cons;
    cons.name = name;
    cons.sense = sense;
    cons.rhs = rhs;
    cons.range_upper = range_upper;
    cons.index = new_idx;
    cons.terms = terms;

    constraints_.push_back(cons);
    cons_name_to_idx_[name] = new_idx;
    return new_idx;
}

const Constraint& LPModel::get_constraint(index_t index) const {
    if (index < 0 || static_cast<size_t>(index) >= constraints_.size()) {
        throw std::out_of_range("Constraint index out of range: " + std::to_string(index));
    }
    return constraints_[static_cast<size_t>(index)];
}

Constraint& LPModel::get_constraint(index_t index) {
    if (index < 0 || static_cast<size_t>(index) >= constraints_.size()) {
        throw std::out_of_range("Constraint index out of range: " + std::to_string(index));
    }
    return constraints_[static_cast<size_t>(index)];
}

const Constraint& LPModel::get_constraint(const std::string& name) const {
    auto it = cons_name_to_idx_.find(name);
    if (it == cons_name_to_idx_.end()) {
        throw std::invalid_argument("Constraint name not found: " + name);
    }
    return constraints_[static_cast<size_t>(it->second)];
}

bool LPModel::has_constraint(const std::string& name) const {
    return cons_name_to_idx_.find(name) != cons_name_to_idx_.end();
}

void LPModel::set_coeff(index_t constraint_idx, index_t var_idx, real_t value) {
    if (constraint_idx < 0 || static_cast<size_t>(constraint_idx) >= constraints_.size()) {
        throw std::out_of_range("Constraint index out of range: " + std::to_string(constraint_idx));
    }
    if (var_idx < 0 || static_cast<size_t>(var_idx) >= variables_.size()) {
        throw std::out_of_range("Variable index out of range: " + std::to_string(var_idx));
    }

    auto& terms = constraints_[static_cast<size_t>(constraint_idx)].terms;
    for (auto& term : terms) {
        if (term.first == var_idx) {
            term.second = value;
            return;
        }
    }

    // Term doesn't exist yet, append
    if (value != 0.0) {
        terms.push_back({var_idx, value});
    }
}

real_t LPModel::get_coeff(index_t constraint_idx, index_t var_idx) const {
    if (constraint_idx < 0 || static_cast<size_t>(constraint_idx) >= constraints_.size()) {
        throw std::out_of_range("Constraint index out of range: " + std::to_string(constraint_idx));
    }
    if (var_idx < 0 || static_cast<size_t>(var_idx) >= variables_.size()) {
        throw std::out_of_range("Variable index out of range: " + std::to_string(var_idx));
    }

    const auto& terms = constraints_[static_cast<size_t>(constraint_idx)].terms;
    for (const auto& term : terms) {
        if (term.first == var_idx) {
            return term.second;
        }
    }
    return 0.0;
}

size_t LPModel::num_nonzeros() const {
    size_t nnz = 0;
    for (const auto& cons : constraints_) {
        for (const auto& term : cons.terms) {
            if (term.second != 0.0) {
                nnz++;
            }
        }
    }
    return nnz;
}

void LPModel::clear() {
    name_ = "unnamed_model";
    sense_ = ObjectiveSense::MINIMIZE;
    obj_offset_ = 0.0;
    variables_.clear();
    constraints_.clear();
    var_name_to_idx_.clear();
    cons_name_to_idx_.clear();
}

} // namespace bharatopt
