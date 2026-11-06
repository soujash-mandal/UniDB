#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <variant>

namespace unidb::sql {
namespace {

TEST(SqlParserCaseTest, ParsesSearchedCaseWithMultipleWhenBranches) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "SELECT CASE WHEN id > 10 THEN 'high' WHEN id = 10 THEN 'ten' "
      "ELSE 'low' END AS label, id FROM users");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(select.columnNames.size(), 2U);
  ASSERT_EQ(select.caseExpressions.size(), 2U);
  ASSERT_TRUE(select.caseExpressions[0].has_value());
  EXPECT_FALSE(select.caseExpressions[1].has_value());
  ASSERT_EQ(select.caseExpressions[0]->branches.size(), 2U);
  EXPECT_EQ(std::get<std::string>(
                select.caseExpressions[0]->branches[0].result),
            "high");
  EXPECT_EQ(std::get<std::string>(select.caseExpressions[0]->elseResult),
            "low");
  ASSERT_TRUE(select.columnAliases[0].has_value());
  EXPECT_EQ(*select.columnAliases[0], "label");
}

TEST(SqlParserCaseTest, RejectsCaseWithoutWhenOrElse) {
  SqlParserImpl parser;
  for (const auto *sql : {
           "SELECT CASE ELSE 1 END FROM users",
           "SELECT CASE WHEN id = 1 THEN 1 END FROM users",
           "SELECT CASE WHEN id = 1 THEN 1 ELSE 2 FROM users",
           "SELECT CASE WHEN id = 1 THEN 1 ELSE 'two' END FROM users",
       }) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
  }
}

TEST(SqlParserCaseTest, RejectsMalformedCaseConditionsAndResults) {
  SqlParserImpl parser;
  for (const auto *sql : {
           "SELECT CASE WHEN THEN 1 ELSE 2 END FROM users",
           "SELECT CASE WHEN id = 1 1 ELSE 2 END FROM users",
           "SELECT CASE WHEN id = 1 THEN ELSE 2 END FROM users",
           "SELECT CASE WHEN id = 1 THEN 1 ELSE END FROM users",
       }) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
  }
}

} // namespace
} // namespace unidb::sql
