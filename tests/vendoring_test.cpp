// This file is part of KAssert.
//
// Copyright 2025 The KAssert Authors

#include <gtest/gtest.h>

#include <kassert/kassert.hpp>
#include <mykassert/kassert.hpp>

TEST(KassertTest, vendored_kassert_is_independent) {
    // we set the levels using the CMake properties for both kassert versions
    // Let's check that they are correctly set and independent
    EXPECT_EQ(KASSERT_ASSERTION_LEVEL, 30);
    EXPECT_TRUE(kassert::internal::assertion_enabled(10));
    EXPECT_TRUE(kassert::internal::assertion_enabled(30));
    
    EXPECT_EQ(MYKASSERT_ASSERTION_LEVEL, 10);
    EXPECT_TRUE(mykassert::internal::assertion_enabled(10));
    EXPECT_FALSE(mykassert::internal::assertion_enabled(30));
}
