// This file is part of KAssert.
//
// Copyright 2021 The KAssert Authors

// Overwrite build option and set assertion level to normal
#undef KASSERT_ASSERTION_LEVEL
#define KASSERT_ASSERTION_LEVEL 30 // up to normal assertions

// Levels for testing
#define ASSERTION_LEVEL_LOWER_THAN_NORMAL  -10000
#define ASSERTION_LEVEL_HIGHER_THAN_NORMAL 10000

#include <csignal>

#include <gmock/gmock.h>

#include "kassert/kassert.hpp"

using namespace ::testing;

/// @brief Custom expectation for testing if a KASSERT fails.
#define EXPECT_KASSERT_FAILS(CODE, FAILURE_MESSAGE) \
    EXPECT_EXIT({ CODE; }, testing::KilledBySignal(SIGABRT), FAILURE_MESSAGE);

/// @brief Custom assertion for testing if a KASSERT fails.
#define ASSERT_KASSERT_FAILS(CODE, FAILURE_MESSAGE) \
    ASSERT_EXIT({ CODE; }, testing::KilledBySignal(SIGABRT), FAILURE_MESSAGE);

// Dummy assertion levels for tests
namespace assert {
constexpr int light = kassert::assert::normal - 1;
constexpr int heavy = kassert::assert::normal + 1;
} // namespace assert

// General comment: all KASSERT() and THROWING_KASSERT() calls with a relation in their expression are placed inside
// lambdas, which are then called from EXPECT_EXIT(). This indirection is necessary as otherwise, GCC does not suppress
// the warning on missing parentheses. This happens whenever the KASSERT() call is passed through two levels of macros,
// i.e.,
//
// #defined A(stmt) B(stmt)
// #defined B(stmt) stmt;
//
// A(KASSERT(false)); // warning not suppressed (with GCC only)
// B(KASSERT(false)); // warning suppress, code ok

TEST(KassertTest, kassert_overloads_compile) {
    // test that all KASSERT overloads compile
    EXPECT_EXIT(
        { KASSERT(false, "__false_is_false_3__", assert::light); },
        KilledBySignal(SIGABRT),
        "__false_is_false_3__"
    );
    EXPECT_EXIT({ KASSERT(false, "__false_is_false_2__"); }, KilledBySignal(SIGABRT), "__false_is_false_2__");
    EXPECT_EXIT({ KASSERT(false); }, KilledBySignal(SIGABRT), "");
}

TEST(KassertTestingHelperTest, kassert_testing_helper) {
    auto failing_function = [] {
        KASSERT(false, "__false_is_false_1__");
    };

    // Pass a single function call to the macro.
    EXPECT_KASSERT_FAILS(failing_function(), "__false_is_false_1");
    ASSERT_KASSERT_FAILS(failing_function(), "__false_is_false_1");

    // Pass a code block to the macro.
    EXPECT_KASSERT_FAILS({ failing_function(); }, "__false_is_false_1");
    ASSERT_KASSERT_FAILS({ failing_function(); }, "__false_is_false_1");
}

// Since we explicitly set the assertion level to normal, heavier assertions should not trigger.
TEST(KassertTest, kassert_respects_assertion_level) {
    EXPECT_EXIT({ KASSERT(false, "", assert::light); }, KilledBySignal(SIGABRT), "");
    KASSERT(false, "", assert::heavy);
}

TEST(KassertTest, kthrow_overloads_compile) {
#ifdef KASSERT_EXCEPTION_MODE
    // test that all THROWING_KASSERT() overloads compile
    EXPECT_THROW({ THROWING_KASSERT(false, "__false_is_false_2__"); }, kassert::KassertException);
    EXPECT_THROW({ THROWING_KASSERT(false); }, kassert::KassertException);
#else  // KASSERT_EXCEPTION_MODE
    EXPECT_EXIT({ THROWING_KASSERT(false, "__false_is_false_2__"); }, KilledBySignal(SIGABRT), "__false_is_false_2__");
    EXPECT_EXIT({ THROWING_KASSERT(false); }, KilledBySignal(SIGABRT), "");
#endif // KASSERT_EXCEPTION_MODE
}

class ZeroCustomArgException : public std::exception {
public:
    ZeroCustomArgException(std::string) {}

    char const* what() const throw() final {
        return "ZeroCustomArgException";
    }
};

class SingleCustomArgException : public std::exception {
public:
    SingleCustomArgException(std::string, int) {}

    char const* what() const throw() final {
        return "SingleCustomArgException";
    }
};

TEST(KassertTest, kthrow_custom_compiles) {
#ifdef KASSERT_EXCEPTION_MODE
    EXPECT_THROW({ THROWING_KASSERT_SPECIFIED(false, "", ZeroCustomArgException); }, ZeroCustomArgException);
    EXPECT_THROW({ THROWING_KASSERT_SPECIFIED(false, "", SingleCustomArgException, 43); }, SingleCustomArgException);
#else  // KASSERT_EXCEPTION_MODE
    EXPECT_EXIT(
        { THROWING_KASSERT_SPECIFIED(false, "", ZeroCustomArgException); },
        KilledBySignal(SIGABRT),
        "ZeroCustomArgException"
    );
    EXPECT_EXIT(
        { THROWING_KASSERT_SPECIFIED(false, "", SingleCustomArgException, 43); },
        KilledBySignal(SIGABRT),
        "SingleCustomArgException"
    );
#endif // KASSERT_EXCEPTION_MODE
}

// Check that THROWING_KASSERT does nothing if the expression evaluates to true.
TEST(KassertTest, kthrow_does_nothing_on_true_expression) {
    THROWING_KASSERT(true);
    THROWING_KASSERT(true, "");
    THROWING_KASSERT_SPECIFIED(true, "", ZeroCustomArgException);
}

// Test that expressions are evaluated as expected
// The following tests do not check the expression expansion!

TEST(KassertTest, unary_true_expressions) {
    // unary expressions that evaluate to true and thus should not trigger the assertions

    // literals
    KASSERT(true);
    KASSERT(!false);

    // variables
    bool const var_true  = true;
    bool const var_false = false;
    KASSERT(var_true);
    KASSERT(!var_false);

    // function calls
    auto id = [](bool const ans) {
        return ans;
    };
    KASSERT(id(true));
    KASSERT(!id(false));

    // unary expressions with implicit conversion to true
    KASSERT(10);
    KASSERT(-10);
    KASSERT(1 + 1); // unary expression from KASSERT perspective
}

TEST(KassertTest, unary_false_expressions) {
    // test unary expressions that evaluate to false and should thus trigger the assertion

    // literals
    EXPECT_EXIT({ KASSERT(false); }, KilledBySignal(SIGABRT), "");
    EXPECT_EXIT({ KASSERT(!true); }, KilledBySignal(SIGABRT), "");

    // variables
    bool const var_true  = true;
    bool const var_false = false;
    EXPECT_EXIT({ KASSERT(var_false); }, KilledBySignal(SIGABRT), "");
    EXPECT_EXIT({ KASSERT(!var_true); }, KilledBySignal(SIGABRT), "");

    // functions
    auto id = [](bool const ans) {
        return ans;
    };
    EXPECT_EXIT({ KASSERT(id(false)); }, KilledBySignal(SIGABRT), "");
    EXPECT_EXIT({ KASSERT(!id(true)); }, KilledBySignal(SIGABRT), "");

    // expressions implicitly convertible to bool
    EXPECT_EXIT({ KASSERT(0); }, KilledBySignal(SIGABRT), "");
    // EXPECT_EXIT({ KASSERT(nullptr); }, KilledBySignal(SIGABRT), ""); -- std::nullptr_t is not convertible to bool
    EXPECT_EXIT({ KASSERT(1 - 1); }, KilledBySignal(SIGABRT), ""); // unary expression from KASSERT perspective
}

TEST(KassertTest, true_arithmetic_relation_expressions) {
    KASSERT(1 == 1);
    KASSERT(1 != 2);
    KASSERT(1 < 2);
    KASSERT(2 > 1);
    KASSERT(1 <= 2);
    KASSERT(2 >= 1);
}

TEST(KassertTest, false_arithmetic_relation_expressions) {
    auto eq = [] {
        KASSERT(1 == 2);
    };
    auto neq = [] {
        KASSERT(1 != 1);
    };
    auto lt = [] {
        KASSERT(1 < 1);
    };
    auto gt = [] {
        KASSERT(1 > 1);
    };
    auto le = [] {
        KASSERT(2 <= 1);
    };
    auto ge = [] {
        KASSERT(1 >= 2);
    };
    EXPECT_EXIT({ eq(); }, KilledBySignal(SIGABRT), "");
    EXPECT_EXIT({ neq(); }, KilledBySignal(SIGABRT), "");
    EXPECT_EXIT({ lt(); }, KilledBySignal(SIGABRT), "");
    EXPECT_EXIT({ gt(); }, KilledBySignal(SIGABRT), "");
    EXPECT_EXIT({ le(); }, KilledBySignal(SIGABRT), "");
    EXPECT_EXIT({ ge(); }, KilledBySignal(SIGABRT), "");
}

TEST(KassertTest, true_chained_relation_ops) {
    KASSERT(1 == 1 == 1);
    KASSERT(1 == 1 != 0);
    KASSERT(1 == 1 & 1);
    KASSERT(5 == 0 | 1);
    KASSERT(5 == 0 ^ 1);
    KASSERT(5 == 5 ^ false);
}

// Test expression expansion of primitive types

TEST(KassertTest, primitive_type_expansion) {
    // arithmetic operators
    auto generic_eq = [](auto const lhs, auto const rhs) {
        KASSERT(lhs == rhs);
    };
    auto generic_gt = [](auto const lhs, auto const rhs) {
        KASSERT(lhs > rhs);
    };
    auto generic_ge = [](auto const lhs, auto const rhs) {
        KASSERT(lhs >= rhs);
    };
    auto generic_lt = [](auto const lhs, auto const rhs) {
        KASSERT(lhs < rhs);
    };
    auto generic_le = [](auto const lhs, auto const rhs) {
        KASSERT(lhs <= rhs);
    };

    EXPECT_EXIT({ generic_eq(1, 2); }, KilledBySignal(SIGABRT), "1 == 2");
    EXPECT_EXIT({ generic_gt(1, 2); }, KilledBySignal(SIGABRT), "1 > 2");
    EXPECT_EXIT({ generic_ge(1, 2); }, KilledBySignal(SIGABRT), "1 >= 2");
    EXPECT_EXIT({ generic_lt(2, 1); }, KilledBySignal(SIGABRT), "2 < 1");
    EXPECT_EXIT({ generic_le(2, 1); }, KilledBySignal(SIGABRT), "2 <= 1");
}

TEST(KassertTest, primitive_type_expansion_limitations) {
    // negation + relation
    auto generic_neg_eq = [](auto const lhs_neg, auto const rhs) {
        KASSERT(!lhs_neg == rhs);
    };

    EXPECT_EXIT({ generic_neg_eq(5, 10); }, KilledBySignal(SIGABRT), "false == 10"); // cannot expand !lhs_neg
}

TEST(KassertTest, chained_rel_ops_expansion) {
    auto generic_chained_eq = [](auto const val1, auto const val2, auto const val3) {
        KASSERT(val1 == val2 == val3);
    };
    auto generic_chained_eq_neq = [](auto const val1, auto const val2, auto const val3) {
        KASSERT(val1 == val2 != val3);
    };
    auto generic_chained_eq_binary_and = [](auto const val1, auto const val2, auto const val3) {
        KASSERT(val1 == val2 & val3);
    };
    auto generic_chained_eq_binary_or = [](auto const val1, auto const val2, auto const val3) {
        KASSERT(val1 == val2 | val3);
    };
    auto generic_chained_eq_binary_xor = [](auto const val1, auto const val2, auto const val3) {
        KASSERT(val1 == val2 ^ val3);
    };

    EXPECT_EXIT({ generic_chained_eq(1, 1, 5); }, KilledBySignal(SIGABRT), "1 == 1 == 5");
    EXPECT_EXIT({ generic_chained_eq_neq(1, 1, 1); }, KilledBySignal(SIGABRT), "1 == 1 != 1");
    EXPECT_EXIT({ generic_chained_eq_binary_and(5, 5, 0); }, KilledBySignal(SIGABRT), "5 == 5 & 0");
    EXPECT_EXIT({ generic_chained_eq_binary_or(5, 4, 0); }, KilledBySignal(SIGABRT), "5 == 4 \\| 0");
    EXPECT_EXIT({ generic_chained_eq_binary_xor(5, 4, 0); }, KilledBySignal(SIGABRT), "5 == 4 \\^ 0");
}

// Test expression expansion of library-supported types

TEST(KassertTest, true_complex_expanded_types) {
    std::vector<int> vec_rhs = {1, 2, 3};
    std::vector<int> vec_lhs = {1, 2, 3};
    KASSERT(vec_rhs == vec_lhs);

    std::pair<int, std::vector<int>> pair_vec_rhs = {1, {2, 3}};
    std::pair<int, std::vector<int>> pair_vec_lhs = {1, {2, 3}};
    KASSERT(pair_vec_rhs == pair_vec_lhs);
}

TEST(KassertTest, empty_and_single_int_vector_expansion) {
    std::vector<int> lhs = {};
    std::vector<int> rhs = {0};

    auto eq = [&] {
        KASSERT(lhs == rhs);
    };

    EXPECT_EXIT({ eq(); }, KilledBySignal(SIGABRT), "\\[\\] == \\[0\\]");
}

TEST(KassertTest, multi_element_int_vector_expansion) {
    std::vector<int> lhs = {1, 2, 3};
    std::vector<int> rhs = {1, 2};

    auto eq = [&] {
        KASSERT(lhs == rhs);
    };

    EXPECT_EXIT({ eq(); }, KilledBySignal(SIGABRT), "\\[1, 2, 3\\] == \\[1, 2\\]");
}

TEST(KassertTest, int_int_pair_expansion) {
    std::pair<int, int> lhs = {1, 2};
    std::pair<int, int> rhs = {1, 3};

    auto eq = [&] {
        KASSERT(lhs == rhs);
    };
    EXPECT_EXIT({ eq(); }, KilledBySignal(SIGABRT), "\\(1, 2\\) == \\(1, 3\\)");
}

TEST(KassertTest, int_int_pair_vector_expansion) {
    std::vector<std::pair<int, int>> lhs = {{1, 2}, {1, 3}};
    std::vector<std::pair<int, int>> rhs = {{1, 2}, {1, 4}};

    auto eq = [&] {
        KASSERT(lhs == rhs);
    };

    EXPECT_EXIT({ eq(); }, KilledBySignal(SIGABRT), "\\[\\(1, 2\\), \\(1, 3\\)\\] == \\[\\(1, 2\\), \\(1, 4\\)\\]");
}

TEST(KassertTest, int_vector_int_pair_expansion) {
    std::pair<std::vector<int>, int> lhs = {{}, 0};
    std::pair<std::vector<int>, int> rhs = {{1}, 1};

    auto eq = [&] {
        KASSERT(lhs == rhs);
    };

    EXPECT_EXIT({ eq(); }, KilledBySignal(SIGABRT), "\\(\\[\\], 0\\) == \\(\\[1\\], 1\\)");
}

// Test expansion of unsupported custom type

TEST(KassertTest, unsupported_type_expansion) {
    struct CustomType {
        bool operator==(CustomType const&) const {
            return false;
        }

        bool operator==(int) const {
            return false;
        }
    };

    auto eq = [] {
        KASSERT(CustomType{} == CustomType{});
    };
    auto eq_int = [](int const val) {
        KASSERT(CustomType{} == val);
    };

    EXPECT_EXIT({ eq(); }, KilledBySignal(SIGABRT), "<\\?> == <\\?>");
    EXPECT_EXIT({ eq_int(42); }, KilledBySignal(SIGABRT), "<\\?> == 42");
}

// Test that short-circuit evaluation works

TEST(KassertTest, short_circuit_evaluation_works) {
    bool flag        = false;
    auto side_effect = [&](bool const ans) {
        flag = true;
        return ans;
    };

    // short-circuit or
    KASSERT(true || side_effect(false));
    EXPECT_FALSE(flag);
    flag = false;

    // do not short-circuit in negative case
    KASSERT(false || side_effect(true));
    EXPECT_TRUE(flag);
    flag = false;

    // short-circuit and
    auto and_sc = [&] {
        KASSERT(false && side_effect(false), "flag=" << flag);
    };
    EXPECT_EXIT({ and_sc(); }, KilledBySignal(SIGABRT), "flag=false");
    flag = false;

    // do not short-circuit in positive case
    KASSERT(true && side_effect(true));
    EXPECT_TRUE(flag);
    flag = false;

    // multiple ors
    KASSERT(false || true || side_effect(false));
    EXPECT_FALSE(flag);
    flag = false;

    // multiple ands
    auto and_and_sc = [&] {
        KASSERT(true && false && side_effect(false), "flag=" << flag);
    };
    EXPECT_EXIT({ and_and_sc(); }, KilledBySignal(SIGABRT), "flag=false");

    // Binary expression + && no short circuit
    KASSERT(1 + 1 == 2 && side_effect(true));
    EXPECT_TRUE(flag);
    flag = false;

    // Binary expression + || with short circuit
    KASSERT(1 + 1 == 2 || side_effect(false));
    EXPECT_FALSE(flag);
    flag = false;
}

// Test that KASSERT_ENABLED disables code

TEST(KassertTest, kassert_enabled_works) {
    bool flag = false;

    // should not be compiled
#if KASSERT_ENABLED(ASSERTION_LEVEL_HIGHER_THAN_NORMAL)
    flag = true;
#endif
    EXPECT_FALSE(flag);
    flag = false;

    // should be compiled
#if KASSERT_ENABLED(KASSERT_ASSERTION_LEVEL_NORMAL)
    flag = true;
#endif
    EXPECT_TRUE(flag);
    flag = false;

    // should be compiled
#if KASSERT_ENABLED(ASSERTION_LEVEL_LOWER_THAN_NORMAL)
    flag = true;
#endif
    EXPECT_TRUE(flag);
    flag = false;
}
