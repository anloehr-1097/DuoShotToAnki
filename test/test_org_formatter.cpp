#include <gtest/gtest.h>

#include <string>

#include "org_formatter.h"

TEST(OrgFormatterTest, GetCurrentDateReturnsNonEmptyString) {
    std::string date = get_current_date();
    EXPECT_FALSE(date.empty());
}

TEST(OrgFormatterTest, GetCurrentDateWithDayReturnsNonEmptyString) {
    std::string date = get_current_date_with_day();
    EXPECT_FALSE(date.empty());
}

TEST(OrgFormatterTest, GetCurrentDateWithDayContainsDate) {
    std::string date = get_current_date_with_day();
    std::string date_only = get_current_date();
    EXPECT_NE(date.find(date_only), std::string::npos);
}
