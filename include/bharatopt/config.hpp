#ifndef BHARATOPT_CONFIG_HPP
#define BHARATOPT_CONFIG_HPP

#include <cstddef>
#include <limits>
#include <algorithm>

#if defined(__has_include)
  #if __has_include(<filesystem>)
    #include <filesystem>
  #elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    namespace std { namespace filesystem = experimental::filesystem; }
  #endif
#endif

namespace bharatopt {

// Precision configuration (default double precision for optimization arithmetic)
using real_t = double;
using index_t = int;
using size_t = std::size_t;

// Portable clamp helper for C++14/C++17 cross-compiler compatibility
template <typename T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : ((hi < v) ? hi : v);
}

// Numerical tolerances
constexpr real_t DEFAULT_FEASIBILITY_TOLERANCE = 1e-7;
constexpr real_t DEFAULT_OPTIMALITY_TOLERANCE  = 1e-7;
constexpr real_t DEFAULT_INTEGRALITY_TOLERANCE = 1e-5;
constexpr real_t DEFAULT_PIVOT_TOLERANCE       = 1e-10;
constexpr real_t DEFAULT_ZERO_TOLERANCE        = 1e-12;
constexpr real_t BHARATOPT_INFINITY            = std::numeric_limits<real_t>::infinity();

// Feature compile flags
#if defined(_WIN32) || defined(_WIN64)
    #define BHARATOPT_OS_WINDOWS
#elif defined(__linux__)
    #define BHARATOPT_OS_LINUX
#elif defined(__APPLE__)
    #define BHARATOPT_OS_MACOS
#endif

} // namespace bharatopt

#endif // BHARATOPT_CONFIG_HPP
