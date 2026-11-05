#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <variant>

namespace unidb::sql {
namespace {

TEST(SqlParserTest, ParsesAggregateFunctionsAndAliases) {
  SqlParserImpl parser;
  const auto result = parser.parse("SELECT COUNT(*) AS total FROM users");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select = std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_TRUE(select.aggregate.has_value());
  EXPECT_EQ(select.aggregate->function, AggregateFunction::Count);
  EXPECT_FALSE(select.aggregate->columnName.has_value());
  ASSERT_TRUE(select.aggregate->alias.has_value());
  EXPECT_EQ(*select.aggregate->alias, "total");

  const auto average = parser.parse("SELECT AVG(age) FROM users");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(average));
  const auto &averageSelect =
      std::get<SelectStatement>(std::get<SqlStatement>(average));
  ASSERT_TRUE(averageSelect.aggregate.has_value());
  EXPECT_EQ(averageSelect.aggregate->function, AggregateFunction::Avg);
  ASSERT_TRUE(averageSelect.aggregate->columnName.has_value());
  EXPECT_EQ(*averageSelect.aggregate->columnName, "age");
}

TEST(SqlParserTest, RejectsMalformedAggregateExpressions) {
  SqlParserImpl parser;
  for (const auto *sql : {"SELECT COUNT FROM users",
                          "SELECT SUM(*) FROM users",
                          "SELECT COUNT( FROM users",
                          "SELECT MIN() FROM users",
                          "SELECT COUNT(*) AS FROM users"}) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
  }
}

} // namespace
} // namespace unidb::sql
