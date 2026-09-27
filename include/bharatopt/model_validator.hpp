#ifndef BHARATOPT_MODEL_VALIDATOR_HPP
#define BHARATOPT_MODEL_VALIDATOR_HPP

#include <string>
#include <vector>
#include <bharatopt/config.hpp>
#include <bharatopt/lp_model.hpp>

namespace bharatopt {

enum class IssueSeverity {
    ERROR,
    WARNING
};

struct ValidationIssue {
    IssueSeverity severity{IssueSeverity::ERROR};
    std::string category;     // "STRUCTURE", "VARIABLE", "CONSTRAINT", "OBJECTIVE", "COEFFICIENT"
    std::string message;      // Diagnostic description
    std::string entity_name;  // Name of variable or constraint
    index_t entity_index{-1}; // Index if applicable

    std::string to_string() const;
};

class ValidationResult {
public:
    bool is_valid() const { return error_count_ == 0; }
    size_t error_count() const { return error_count_; }
    size_t warning_count() const { return warning_count_; }

    const std::vector<ValidationIssue>& issues() const { return issues_; }
    std::vector<ValidationIssue> errors() const;
    std::vector<ValidationIssue> warnings() const;

    void add_issue(IssueSeverity severity,
                   const std::string& category,
                   const std::string& message,
                   const std::string& entity_name = "",
                   index_t entity_index = -1);

    void add_error(const std::string& category,
                   const std::string& message,
                   const std::string& entity_name = "",
                   index_t entity_index = -1) {
        add_issue(IssueSeverity::ERROR, category, message, entity_name, entity_index);
    }

    void add_warning(const std::string& category,
                     const std::string& message,
                     const std::string& entity_name = "",
                     index_t entity_index = -1) {
        add_issue(IssueSeverity::WARNING, category, message, entity_name, entity_index);
    }

    std::string to_string() const;

private:
    std::vector<ValidationIssue> issues_;
    size_t error_count_{0};
    size_t warning_count_{0};
};

class ModelValidator {
public:
    ModelValidator() = default;

    // Read-only model validation
    ValidationResult validate(const LPModel& model) const;
};

} // namespace bharatopt

#endif // BHARATOPT_MODEL_VALIDATOR_HPP
