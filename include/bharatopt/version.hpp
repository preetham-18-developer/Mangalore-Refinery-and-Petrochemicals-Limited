#ifndef BHARATOPT_VERSION_HPP
#define BHARATOPT_VERSION_HPP

#include <string>

namespace bharatopt {

constexpr int VERSION_MAJOR = 0;
constexpr int VERSION_MINOR = 1;
constexpr int VERSION_PATCH = 0;
constexpr const char* VERSION_STRING = "0.1.0-alpha";

inline std::string get_version_info() {
    return std::string("BharatOpt Solver v") + VERSION_STRING + " (SIH 2026 PS 26119)";
}

} // namespace bharatopt

#endif // BHARATOPT_VERSION_HPP
