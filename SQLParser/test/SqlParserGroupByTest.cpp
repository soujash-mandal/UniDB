#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <variant>

namespace unidb::sql {
namespace {

TEST(SqlParserTest, ParsesGroupByAndHavingAggregate) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "SELECT department, COUNT(*) AS total FROM employees "
      "WHERE active = 1 GROUP BY department HAVING COUNT(*) >= 2");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select = std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(select.columnNames.size(), 1U);
  EXPECT_EQ(select.columnNames[0], "department");
  ASSERT_TRUE(select.aggregate.has_value());
  EXPECT_EQ(select.aggregate->function, AggregateFunction::Count);
  ASSERT_EQ(select.groupBy.size(), 1U);
  EXPECT_EQ(select.groupBy[0], "department");
  ASSERT_TRUE(select.having.has_value());
  EXPECT_EQ(select.having->aggregate.function, AggregateFunction::Count);
  EXPECT_EQ(select.having->comparison, ComparisonOperator::GreaterThanOrEqual);
  EXPECT_EQ(std::get<std::int64_t>(select.having->value), 2);
}

TEST(SqlParserTest, RejectsMalformedGroupByAndHaving) {
  SqlParserImpl parser;
  for (const auto *sql : {
           "SELECT department, COUNT(*) FROM employees GROUP department",
           "SELECT department, COUNT(*) FROM employees GROUP BY",
           "SELECT department, COUNT(*) FROM employees HAVING COUNT(*)",
           "SELECT department, COUNT(*) FROM employees GROUP BY department "
           "HAVING SUM(*) > 1"}) {
    EXPECT_TRUE(std::holds_alternative<SqlParseError>(parser.parse(sql)))
        << sql;
  }
}

} // namespace
} // namespace unidb::sql
