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
/// @brief Provides macros to implement the KASSERT, THROWING_KASSERT and THROWING_KASSERT_SPECIFIED macros.

#pragma once

/// @cond IMPLEMENTATION

// To decompose expressions, the KASSERT_KASSERT_HPP_ASSERT_IMPL() produces code such as
//
// Decomposer{} <= a == b       [ with implicit parentheses: ((Decomposer{} <= a) == b) ]
//
// This triggers a warning with -Wparentheses, suggesting to set explicit parentheses, which is impossible in this
// situation. Thus, we use compiler-specific _Pragmas to suppress these warning.
// Note that warning suppression in GCC does not work if the KASSERT() call is passed through >= two macro calls:
//
// #define A(stmt) B(stmt)
// #define B(stmt) stmt;
// A(KASSERT(1 != 1)); -- warning suppression does not work
//
// This is a known limitation of the current implementation.
#if defined(__GNUC__) && !defined(__clang__) // GCC
    #define KASSERT_KASSERT_HPP_DIAGNOSTIC_PUSH               _Pragma("GCC diagnostic push")
    #define KASSERT_KASSERT_HPP_DIAGNOSTIC_POP                _Pragma("GCC diagnostic pop")
    #define KASSERT_KASSERT_HPP_DIAGNOSTIC_IGNORE_PARENTHESES _Pragma("GCC diagnostic ignored \"-Wparentheses\"")
#elif defined(__clang__) // Clang
    #define KASSERT_KASSERT_HPP_DIAGNOSTIC_PUSH               _Pragma("clang diagnostic push")
    #define KASSERT_KASSERT_HPP_DIAGNOSTIC_POP                _Pragma("clang diagnostic pop")
    #define KASSERT_KASSERT_HPP_DIAGNOSTIC_IGNORE_PARENTHESES _Pragma("clang diagnostic ignored \"-Wparentheses\"")
#else // Other compilers -> no supression supported
    #define KASSERT_KASSERT_HPP_DIAGNOSTIC_PUSH
    #define KASSERT_KASSERT_HPP_DIAGNOSTIC_POP
    #define KASSERT_KASSERT_HPP_DIAGNOSTIC_IGNORE_PARENTHESES
#endif

// This is the actual implementation of the KASSERT() macro.
//
// - Note that expanding the macro into a `do { ... } while(false)` pseudo-loop is a common trick to make a macro
//   "act like a statement". Otherwise, it would have surprising effects if the macro is used inside a `if` branch
//   without braces.
// - If the assertion level is disabled, this should not generate any code (assuming that the compiler removes the
//   dead loop).
// - `evaluate_and_print_assertion` evaluates the assertion and prints an error message if it failed.
// - The call to `std::abort()` is not wrapped in a function to keep the stack trace clean.
#define KASSERT_KASSERT_HPP_KASSERT_IMPL(type, expression, message, level)                           \
    do {                                                                                             \
        if constexpr (kassert::internal::assertion_enabled(level)) {                                 \
            KASSERT_KASSERT_HPP_DIAGNOSTIC_PUSH                                                      \
            KASSERT_KASSERT_HPP_DIAGNOSTIC_IGNORE_PARENTHESES                                        \
            if (!kassert::internal::evaluate_and_print_assertion(                                    \
                    type,                                                                            \
                    kassert::internal::finalize_expr(kassert::internal::Decomposer{} <= expression), \
                    KASSERT_KASSERT_HPP_SOURCE_LOCATION,                                             \
                    #expression                                                                      \
                )) {                                                                                 \
                kassert::Logger<std::ostream&>(std::cerr) << message << "\n";                        \
                std::abort();                                                                        \
            }                                                                                        \
            KASSERT_KASSERT_HPP_DIAGNOSTIC_POP                                                       \
        }                                                                                            \
    } while (false)

// Expands a macro depending on its number of arguments. For instance,
//
// #define FOO(...) KASSERT_KASSERT_HPP_VARARG_HELPER_3(, __VA_ARGS__, IMPL3, IMPL2, IMPL1, dummy)
//
// expands to IMPL3 with 3 arguments, IMPL2 with 2 arguments and IMPL1 with 1 argument.
// To do this, the macro always expands to its 5th argument. Depending on the number of parameters, __VA_ARGS__
// pushes the right implementation to the 5th parameter.
#define KASSERT_KASSERT_HPP_VARARG_HELPER_3(X, Y, Z, W, FUNC, ...) FUNC
#define KASSERT_KASSERT_HPP_VARARG_HELPER_2(X, Y, Z, FUNC, ...)    FUNC

// KASSERT() chooses the right implementation depending on its number of arguments.
#define KASSERT_3(expression, message, level) KASSERT_KASSERT_HPP_KASSERT_IMPL("ASSERTION", expression, message, level)
#define KASSERT_2(expression, message)        KASSERT_3(expression, message, kassert::assert::normal)
#define KASSERT_1(expression)                 KASSERT_2(expression, "")

// Implementation of the THROWING_KASSERT() macro.
// In KASSERT_EXCEPTION_MODE, we throw an exception similar to the implementation of KASSERT(), although expression
// decomposition in exceptions is currently unsupported. Otherwise, the macro delegates to KASSERT().
#ifdef KASSERT_EXCEPTION_MODE
    #define KASSERT_KASSERT_HPP_THROWING_KASSERT_IMPL_INTERNAL(expression, exception_type, message, ...) \
        do {                                                                                             \
            if (!(expression)) {                                                                         \
                throw exception_type(message, ##__VA_ARGS__);                                            \
            }                                                                                            \
        } while (false)
#else
    #define KASSERT_KASSERT_HPP_THROWING_KASSERT_IMPL_INTERNAL(expression, exception_type, message, ...) \
        do {                                                                                             \
            if constexpr (kassert::internal::assertion_enabled(kassert::assert::kthrow)) {               \
                if (!(expression)) {                                                                     \
                    kassert::Logger<std::ostream&>(std::cerr)                                            \
                        << (exception_type(message, ##__VA_ARGS__).what()) << "\n";                      \
                    std::abort();                                                                        \
                }                                                                                        \
            }                                                                                            \
        } while (false)
#endif

#define KASSERT_KASSERT_HPP_THROWING_KASSERT_IMPL(expression, message)                                   \
    KASSERT_KASSERT_HPP_THROWING_KASSERT_IMPL_INTERNAL(                                                  \
        expression,                                                                                      \
        kassert::KassertException,                                                                       \
        kassert::internal::build_what(                                                                   \
            #expression,                                                                                 \
            KASSERT_KASSERT_HPP_SOURCE_LOCATION,                                                         \
            (kassert::internal::RrefOStringstreamLogger{std::ostringstream{}} << message).stream().str() \
        )                                                                                                \
    )

#define KASSERT_KASSERT_HPP_THROWING_KASSERT_CUSTOM_IMPL(expression, exception_type, message, ...)       \
    KASSERT_KASSERT_HPP_THROWING_KASSERT_IMPL_INTERNAL(                                                  \
        expression,                                                                                      \
        exception_type,                                                                                  \
        kassert::internal::build_what(                                                                   \
            #expression,                                                                                 \
            KASSERT_KASSERT_HPP_SOURCE_LOCATION,                                                         \
            (kassert::internal::RrefOStringstreamLogger{std::ostringstream{}} << message).stream().str() \
        ),                                                                                               \
        ##__VA_ARGS__                                                                                    \
    )

// THROWING_KASSERT() chooses the right implementation depending on its number of arguments.
#define THROWING_KASSERT_2(expression, message) KASSERT_KASSERT_HPP_THROWING_KASSERT_IMPL(expression, message)
#define THROWING_KASSERT_1(expression)          THROWING_KASSERT_2(expression, "")

// __PRETTY_FUNCTION__ is a compiler extension supported by GCC and clang that prints more information than __func__
#if defined(__GNUC__) || defined(__clang__)
    #define KASSERT_KASSERT_HPP_FUNCTION_NAME __PRETTY_FUNCTION__
#else
    #define KASSERT_KASSERT_HPP_FUNCTION_NAME __func__
#endif

// Represents the static location in the source code.
#define KASSERT_KASSERT_HPP_SOURCE_LOCATION                   \
    kassert::internal::SourceLocation {                       \
        __FILE__, __LINE__, KASSERT_KASSERT_HPP_FUNCTION_NAME \
    }

/// @endcond
