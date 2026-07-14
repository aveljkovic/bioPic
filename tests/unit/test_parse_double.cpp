#include <gtest/gtest.h>

#include "core/parse_double.hpp"

TEST(ParseDoubleTest, ParsesValidThresholds) {
    const auto value = biopic::detail::parse_double("50.5");
    ASSERT_TRUE(value.has_value());
    EXPECT_DOUBLE_EQ(*value, 50.5);
}

TEST(ParseDoubleTest, RejectsInvalidInput) {
    EXPECT_FALSE(biopic::detail::parse_double("").has_value());
    EXPECT_FALSE(biopic::detail::parse_double("abc").has_value());
    EXPECT_FALSE(biopic::detail::parse_double("12abc").has_value());
    EXPECT_FALSE(biopic::detail::parse_double(" 12").has_value());
    EXPECT_FALSE(biopic::detail::parse_double("12 ").has_value());
}
