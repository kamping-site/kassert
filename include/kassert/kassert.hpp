// This file is part of KAssert.
//
// Copyright 2021-2022 The KAssert Authors
//
// KAssert is free software : you can redistribute it and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
// version. KAssert is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
// for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with KAssert.  If not, see
// <https://www.gnu.org/licenses/>.

/// @file
/// @brief Macros for asserting runtime checks.

#pragma once

#include <iostream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "kassert/internal/assertion_macros.hpp"
#include "kassert/internal/expression_decomposition.hpp"
#include "kassert/internal/logger.hpp"

/// @brief Assertion levels
namespace kassert::assert {
/// @addtogroup assertion-levels Assertion levels
/// @{

/// @brief Assertion level for exceptions if exception mode is disabled.
#define KASSERT_ASSERTION_LEVEL_KTHROW 10

/// @brief Assertion level for exceptions if exception mode is disabled.
constexpr int kthrow = KASSERT_ASSERTION_LEVEL_KTHROW;

/// @brief Default assertion level. This level is used if no assertion level is specified.
#define KASSERT_ASSERTION_LEVEL_NORMAL 30

/// @brief Default assertion level. This level is used if no assertion level is specified.
constexpr int normal = KASSERT_ASSERTION_LEVEL_NORMAL;

/// @}
} // namespace kassert::assert

#ifndef KASSERT_ASSERTION_LEVEL
    #warning "Assertion level was not set explicitly; using default assertion level."
  /// @brief Default assertion level to `kassert::assert::normal` if not set explicitly.
    #define KASSERT_ASSERTION_LEVEL KASSERT_ASSERTION_LEVEL_NORMAL
#endif

/// @brief Assertion macro. Accepts between one and three parameters.
/// @ingroup assertion
///
/// Assertions are enabled or disabled by setting a compile-time assertion level (`-DKASSERT_ASSERTION_LEVEL=<int>`).
/// For predefined assertion levels, see @ref assertion-levels.
/// If an assertion is enabled and fails, the KASSERT() macro prints an expansion of the expression similar to Catch2.
/// This process is described in @ref expression-expansion.
///
/// The macro accepts 1 to 3 parameters:
/// 1. The assertion expression (mandatory).
/// 2. Error message that is printed in addition to the decomposed expression (optional). The message is piped into
/// a logger object. Thus, one can use the `<<` operator to build the error message similar to how one would use
/// `std::cout`.
/// 3. The level of the assertion (optional, default: `kassert::assert::normal`, see @ref assertion-levels).
#define KASSERT(...)                     \
    KASSERT_KASSERT_HPP_VARARG_HELPER_3( \
        ,                                \
        __VA_ARGS__,                     \
        KASSERT_3(__VA_ARGS__),          \
        KASSERT_2(__VA_ARGS__),          \
        KASSERT_1(__VA_ARGS__),          \
        ignore                           \
    )

/// @brief Macro for throwing exceptions. Accepts between one and three parameters.
/// @ingroup assertion
///
/// Exceptions are only used in exception mode, which is enabled by using the CMake option
/// `-DKASSERT_EXCEPTION_MODE=On`. Otherwise, the macro generates a KASSERT() with assertion level
/// `kassert::assert::kthrow` (lowest level).
///
/// The macro accepts 1 to 2 parameters:
/// 1. Expression that causes the exception to be thrown if it evaluates to \c false (mandatory).
/// 2. Error message that is printed in addition to the decomposed expression (optional). The message is piped into
/// a logger object. Thus, one can use the `<<` operator to build the error message similar to how one would use
/// `std::cout`.
#define THROWING_KASSERT(...)            \
    KASSERT_KASSERT_HPP_VARARG_HELPER_2( \
        ,                                \
        __VA_ARGS__,                     \
        THROWING_KASSERT_2(__VA_ARGS__), \
        THROWING_KASSERT_1(__VA_ARGS__), \
        ignore                           \
    )

/// @brief Macro for throwing custom exception.
/// @ingroup assertion
///
/// The macro requires at least 2 parameters:
/// 1. Expression that causes the exception to be thrown if it evaluates to \c false (mandatory).
/// 2. Error message that is printed in addition to the decomposed expression (optional). The message is piped into
/// a logger object. Thus, one can use the `<<` operator to build the error message similar to how one would use
/// `std::cout`.
/// 3. Type of the exception to be used. The exception type must have a ctor that takes a `std::string` as its
/// first argument, followed by any additional parameters passed to this macro.
/// 4, 5, 6, ... Parameters that are forwarded to the exception type's ctor.
///
/// Any other parameter is passed to the constructor of the exception class.
#define THROWING_KASSERT_SPECIFIED(expression, message, exception_type, ...) \
    KASSERT_KASSERT_HPP_THROWING_KASSERT_CUSTOM_IMPL(expression, exception_type, message, ##__VA_ARGS__)

namespace kassert::internal {
/// @brief Describes a source code location.
struct SourceLocation {
    /// @brief Filename.
    char const* file;
    /// @brief Line number.
    unsigned row;
    /// @brief Function name.
    char const* function;
};

/// @brief Builds the description for an exception.
/// @param expression Expression that caused this exception to be thrown.
/// @param where Source code location where the exception was thrown.
/// @param message User message describing this exception.
/// @return The description of this exception.
[[maybe_unused]] inline std::string
build_what(std::string const& expression, SourceLocation const where, std::string const& message) {
    using namespace std::string_literals;
    return "\n"s + where.file + ": In function '" + where.function + "':\n" + where.file + ": "
           + std::to_string(where.row) + ": FAILED ASSERTION\n" + "\t" + expression + "\n" + message + "\n";
}
} // namespace kassert::internal

namespace kassert {
/// @brief The default exception type used together with \c THROWING_KASSERT. Reports the erroneous expression together
/// with a custom error message.
class KassertException : public std::exception {
public:
    /// @brief Constructs the exception
    /// @param message A custom error message.
    explicit KassertException(std::string message) : _what(std::move(message)) {}

    /// @brief Gets a description of this exception.
    /// @return A description of this exception.
    [[nodiscard]] char const* what() const noexcept final {
        return _what.c_str();
    }

private:
    /// @brief The description of this exception.
    std::string _what;
};
} // namespace kassert

namespace kassert::internal {
/// @brief Checks if a assertion of the given level is enabled. This is controlled by the CMake option
/// \c KASSERT_ASSERTION_LEVEL.
/// @param level The level of the assertion.
/// @return Whether the assertion is enabled.
constexpr bool assertion_enabled(int level) {
    return level <= KASSERT_ASSERTION_LEVEL;
}

/// @brief Checks if a assertion of the given level is enabled. This is controlled by the CMake option
/// \c KASSERT_ASSERTION_LEVEL. This is the macro version of assertion_enabled for use in the preprocessor.
/// @param level The level of the assertion.
/// @return Whether the assertion is enabled.
#define KASSERT_ENABLED(level) level <= KASSERT_ASSERTION_LEVEL

/// @brief Evaluates an assertion that could not be decomposed (i.e., expressions that use && or ||). If the assertion
/// fails, prints an error describing the failed assertion.
/// @param type Actual type of this check. In exception mode, this parameter has always value \c ASSERTION, otherwise
/// it names the type of the exception that would have been thrown.
/// @param result Assertion expression result to be checked.
/// @param where Source code location of the assertion.
/// @param expr_str Stringified assertion expression.
/// @return Result of the assertion. If true, the assertion was triggered and the program should be halted.
inline bool
evaluate_and_print_assertion(char const* type, bool result, SourceLocation const& where, char const* expr_str) {
    if (!result) {
        OStreamLogger(std::cerr) << where.file << ": In function '" << where.function << "':\n"
                                 << where.file << ":" << where.row << ": FAILED " << type << "\n"
                                 << "\t" << expr_str << "\n";
    }
    return result;
}

/// @brief Evaluates an assertion expression. If the assertion fails, prints an error describing the failed assertion.
/// @param type Actual type of this check. In exception mode, this parameter has always value \c ASSERTION, otherwise
/// it names the type of the exception that would have been thrown.
/// @param expr Assertion expression to be checked.
/// @param where Source code location of the assertion.
/// @param expr_str Stringified assertion expression.
/// @return Result of the assertion. If true, the assertion was triggered and the program should be halted.
inline bool
evaluate_and_print_assertion(char const* type, Expression&& expr, SourceLocation const& where, char const* expr_str) {
    if (!expr.result()) {
        OStreamLogger(std::cerr) << where.file << ": In function '" << where.function << "':\n"
                                 << where.file << ":" << where.row << ": FAILED " << type << "\n"
                                 << "\t" << expr_str << "\n"
                                 << "with expansion:\n"
                                 << "\t" << expr << "\n";
    }
    return expr.result();
}
} // namespace kassert::internal
