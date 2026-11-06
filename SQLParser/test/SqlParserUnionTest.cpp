#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <variant>

namespace unidb::sql {
namespace {

TEST(SqlParserUnionTest, ParsesUnionAllWithTwoSelectBranches) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "SELECT name FROM employees UNION ALL SELECT name FROM contractors;");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto *unionAll =
      std::get_if<UnionAllStatement>(&std::get<SqlStatement>(result));
  ASSERT_NE(unionAll, nullptr);
  ASSERT_EQ(unionAll->selects.size(), 2U);
  EXPECT_EQ(unionAll->selects[0].tableName, "employees");
  EXPECT_EQ(unionAll->selects[1].tableName, "contractors");
  EXPECT_EQ(unionAll->selects[0].columnNames[0], "name");
  EXPECT_EQ(unionAll->selects[1].columnNames[0], "name");
}

TEST(SqlParserUnionTest, ParsesMultipleUnionAllBranches) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "SELECT id FROM first_table UNION ALL "
      "SELECT id FROM second_table UNION ALL SELECT id FROM third_table");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto *unionAll =
      std::get_if<UnionAllStatement>(&std::get<SqlStatement>(result));
  ASSERT_NE(unionAll, nullptr);
  EXPECT_EQ(unionAll->selects.size(), 3U);
}

TEST(SqlParserUnionTest, RejectsUnionWithoutAll) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "SELECT id FROM first_table UNION SELECT id FROM second_table");

  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::UnsupportedStatement);
}

TEST(SqlParserUnionTest, RejectsMissingSelectAfterUnionAll) {
  SqlParserImpl parser;
  const auto result = parser.parse("SELECT id FROM first_table UNION ALL");

  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::SyntaxError);
}

} // namespace
} // namespace unidb::sql
