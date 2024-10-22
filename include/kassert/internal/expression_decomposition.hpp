// This file is part of KAssert.
//
// Copyright 2021-2022 The KAssert Authors

/// @file
/// @brief Expression decomposition.
///
/// Failed assertions try to expand the expression similar to what Catch2 does. This is achieved by the following
/// process:
///
/// In a call
/// ```
/// KASSERT(rhs == lhs)
/// ```
/// KASSERT() also prints the values of \c rhs and \c lhs. However, this expression expansion is limited and only works
/// for expressions that do not contain parentheses, but are implicitly left-associative. This is due to its
/// implementation:
/// ```
/// KASSERT(rhs == lhs)
/// ```
/// is replaced by
/// ```
/// Decomposer{} <= rhs == lhs
/// ```
/// which is interpreted by the compiler as
/// ```
/// ((Decomposer{} <= rhs) == lhs)
/// ```
/// where the first <= relation is overloaded to return a proxy object which in turn overloads other operators. If the
/// expression is not implicitly left-associative or contains parentheses, this does not work:
/// ```
/// KASSERT(rhs1 == lhs1 && rhs2 == lhs2)
/// ```
/// is replaced by (with implicit parentheses)
/// ```
/// ((Decomposer{} <= rhs1) == lhs1) && (rhs2 == lhs2))
/// ```
/// Thus, the left hand side of \c && can only be expanded to the *result* of `rhs2 == lhs2`.
/// This limitation only affects the error message, not the interpretation of the expression itself.

#pragma once

#include <type_traits>

#include "kassert/internal/logger.hpp"

namespace kassert::internal {
/// @brief Type trait that is always false, to implement static_asserts that always fail, thus preventing a
/// template function from being instanciated. Used to forbid calling the overloads of && and ||.
/// @tparam T Some template parameter of the template that should never be instantiated.
template <typename T>
struct AlwaysFalse : public std::false_type {};

/// @brief Interface for decomposed unary and binary expressions.
class Expression {
public:
    /// @brief Virtual destructor since we use virtual functions.
    virtual ~Expression() = default;

    /// @brief Evaluate the assertion wrapped in this Expr.
    /// @return The boolean value that the assertion evalutes to.
    [[nodiscard]] virtual bool result() const = 0;

    /// @brief Write this expression with stringified operands to the given assertion logger.
    /// @param out The assertion logger.
    virtual void stringify(OStreamLogger& out) const = 0;

    /// @brief Writes an expression with stringified operands to the given assertion logger.
    /// @param out The assertion logger.
    /// @param expr The expression to be stringified.
    /// @return The assertion logger.
    friend OStreamLogger& operator<<(OStreamLogger& out, Expression const& expr) {
        expr.stringify(out);
        return out;
    }
};

/// @brief A decomposed binary expression.
/// @tparam LhsT Decomposed type of the left hand side of the expression.
/// @tparam RhsT Decomposed type of the right hand side of the expression.
template <typename LhsT, typename RhsT>
class BinaryExpression : public Expression {
public:
    /// @brief Constructs a decomposed binary expression.
    /// @param result Boolean result of the expression.
    /// @param lhs Decomposed left hand side of the expression.
    /// @param op Stringified operator or relation.
    /// @param rhs Decomposed right hand side of the expression.
    BinaryExpression(bool const result, LhsT const& lhs, std::string_view const op, RhsT const& rhs)
        : _result(result),
          _lhs(lhs),
          _op(op),
          _rhs(rhs) {}

    /// @brief The boolean result of the expression. This is used when retrieving the expression result after
    /// decomposition.
    /// @return The boolean result of the expression.
    [[nodiscard]] bool result() const final {
        return _result;
    }

    /// @brief Implicitly cast to bool. This is used when encountering && or ||.
    /// @return The boolean result of the expression.
    operator bool() {
        return _result;
    }

    /// @brief Writes this expression with stringified operands to the given assertion logger.
    /// @param out The assertion logger.
    void stringify(OStreamLogger& out) const final {
        stringify_value(out, _lhs);
        out << " " << _op << " ";
        stringify_value(out, _rhs);
    }

    /// @cond IMPLEMENTATION

    // Overload operators to return a proxy object that decomposes the rhs of the logical operator
#define KASSERT_ASSERT_OP(op)                                                     \
    template <typename RhsPrimeT>                                                 \
    friend BinaryExpression<BinaryExpression<LhsT, RhsT>, RhsPrimeT> operator op( \
        BinaryExpression<LhsT, RhsT>&& lhs,                                       \
        RhsPrimeT const&               rhs_prime                                  \
    ) {                                                                           \
        using namespace std::string_view_literals;                                \
        return BinaryExpression<BinaryExpression<LhsT, RhsT>, RhsPrimeT>(         \
            lhs.result() op rhs_prime,                                            \
            lhs,                                                                  \
            #op##sv,                                                              \
            rhs_prime                                                             \
        );                                                                        \
    }

    KASSERT_ASSERT_OP(&)
    KASSERT_ASSERT_OP(|)
    KASSERT_ASSERT_OP(^)
    KASSERT_ASSERT_OP(==)
    KASSERT_ASSERT_OP(!=)

#undef KASSERT_ASSERT_OP

    /// @endcond

private:
    /// @brief Boolean result of this expression.
    bool _result;
    /// @brief Decomposed left hand side of this expression.
    LhsT const& _lhs;
    /// @brief Stringified operand or relation symbol.
    std::string_view _op;
    /// @brief Right hand side of this expression.
    RhsT const& _rhs;
};

/// @brief Decomposed unary expression.
/// @tparam Lhst Decomposed expression type.
template <typename LhsT>
class UnaryExpression : public Expression {
public:
    /// @brief Constructs this unary expression from an expression.
    /// @param lhs The expression.
    explicit UnaryExpression(LhsT const& lhs) : _lhs(lhs) {}

    /// @brief Evaluates this expression.
    /// @return The boolean result of this expression.
    [[nodiscard]] bool result() const final {
        return static_cast<bool>(_lhs);
    }

    /// @brief Writes this expression with stringified operands to the given assertion logger.
    /// @param out The assertion logger.
    void stringify(OStreamLogger& out) const final {
        stringify_value(out, _lhs);
    }

private:
    /// @brief The expression.
    LhsT const& _lhs;
};

/// @brief The left hand size of a decomposed expression. This can either be turned into a \c BinaryExpr if an operand
/// or relation follows, or into a \c UnaryExpr otherwise.
/// @tparam LhsT The expression type.
template <typename LhsT>
class LhsExpression {
public:
    /// @brief Constructs this left hand size of a decomposed expression.
    /// @param lhs The wrapped expression.
    explicit LhsExpression(LhsT const& lhs) : _lhs(lhs) {}

    /// @brief Turns this expression into an \c UnaryExpr. This might only be called if the wrapped expression is
    /// implicitly convertible to \c bool.
    /// @return This expression as \c UnaryExpr.
    UnaryExpression<LhsT> make_unary() {
        static_assert(std::is_convertible_v<LhsT, bool>, "expression must be convertible to bool");
        return UnaryExpression<LhsT>{_lhs};
    }

    /// @brief Implicitly cast to bool. This is used when encountering && or ||.
    /// @return The boolean result of the expression.
    operator bool() {
        return _lhs;
    }

    /// @cond IMPLEMENTATION

    // Overload binary operators to return a proxy object that decomposes the rhs of the operator.
#define KASSERT_ASSERT_OP(op)                                                               \
    template <typename RhsT>                                                                \
    friend BinaryExpression<LhsT, RhsT> operator op(LhsExpression&& lhs, RhsT const& rhs) { \
        using namespace std::string_view_literals;                                          \
        return {lhs._lhs op rhs, lhs._lhs, #op##sv, rhs};                                   \
    }

    KASSERT_ASSERT_OP(==)
    KASSERT_ASSERT_OP(!=)
    KASSERT_ASSERT_OP(<)
    KASSERT_ASSERT_OP(<=)
    KASSERT_ASSERT_OP(>)
    KASSERT_ASSERT_OP(>=)
    KASSERT_ASSERT_OP(&)
    KASSERT_ASSERT_OP(|)
    KASSERT_ASSERT_OP(^)

#undef KASSERT_ASSERT_OP

    /// @endcond

private:
    /// @brief The wrapped expression.
    LhsT const& _lhs;
};

/// @brief Decomposes an expression (see group description).
struct Decomposer {
    /// @brief Decomposes an expression (see group description).
    /// @tparam LhsT The type of the expression.
    /// @param lhs The left hand side of the expression.
    /// @return \c lhs wrapped in a \c LhsExpr.
    template <typename LhsT>
    friend LhsExpression<LhsT> operator<=(Decomposer&&, LhsT const& lhs) {
        return LhsExpression<LhsT>(lhs);
    }
};

/// @brief If an expression cannot be decomposed (due to && or ||, to preserve short-circuit evaluation), simply return
/// the result of the assertion.
/// @param result Result of the assertion.
/// @return Result of the assertion.
inline bool finalize_expr(bool const result) {
    return result;
}

/// @brief Transforms \c LhsExpression into \c UnaryExpression, does nothing to a \c Expression (see group description).
/// @tparam ExprT Type of the expression, either \c LhsExpression or a \c BinaryExpression.
/// @param expr The expression.
/// @return The expression as some subclass of \c Expression.
template <typename ExprT>
decltype(auto) finalize_expr(ExprT&& expr) {
    if constexpr (std::is_base_of_v<Expression, std::remove_reference_t<std::remove_const_t<ExprT>>>) {
        return std::forward<ExprT>(expr);
    } else {
        return expr.make_unary();
    }
}
} // namespace kassert::internal
