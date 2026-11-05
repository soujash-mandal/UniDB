#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace unidb::sql {
namespace {

TEST(SqlParserTest, ParsesCreateTableCaseInsensitively) {
  SqlParserImpl parser;
  const auto result = parser.parse("create TABLE users (id INT, name text);");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &statement = std::get<SqlStatement>(result);
  ASSERT_TRUE(std::holds_alternative<CreateTableStatement>(statement));
  const auto &create = std::get<CreateTableStatement>(statement);
  EXPECT_EQ(create.tableName, "users");
  ASSERT_EQ(create.columns.size(), 2U);
  EXPECT_EQ(create.columns[0].name, "id");
  EXPECT_EQ(create.columns[0].type, SqlType::Int);
  EXPECT_EQ(create.columns[1].type, SqlType::Text);
}

TEST(SqlParserTest, ParsesInsertWithSignedIntegerAndEscapedText) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("INSERT INTO users (id, name) VALUES (-42, 'O''Reilly')");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &insert =
      std::get<InsertStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(insert.values.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(insert.values[0]), -42);
  EXPECT_EQ(std::get<std::string>(insert.values[1]), "O'Reilly");
}

TEST(SqlParserTest, ParsesSelectAllAndOptionalEqualityPredicate) {
  SqlParserImpl parser;
  const auto result = parser.parse("SELECT * FROM users WHERE id = 7;");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  EXPECT_TRUE(select.selectAll);
  EXPECT_EQ(select.tableName, "users");
  ASSERT_TRUE(std::holds_alternative<WhereEquals>(select.where));
  const auto &where = std::get<WhereEquals>(select.where);
  EXPECT_EQ(std::get<std::int64_t>(where.value), 7);
}

TEST(SqlParserTest, ParsesSelectedColumns) {
  SqlParserImpl parser;
  const auto result = parser.parse("SELECT id, name FROM users");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  EXPECT_FALSE(select.selectAll);
  ASSERT_EQ(select.columnNames.size(), 2U);
  EXPECT_EQ(select.columnNames[0], "id");
  EXPECT_EQ(select.columnNames[1], "name");
}

TEST(SqlParserTest, ReportsMalformedSyntax) {
  SqlParserImpl parser;
  const auto result = parser.parse("SELECT FROM users");

  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::SyntaxError);
}

TEST(SqlParserTest, ReportsUnsupportedStatementAndOperators) {
  SqlParserImpl parser;
  const auto join = parser.parse("SELECT * FROM users JOIN roles");
  ASSERT_TRUE(std::holds_alternative<SqlParseError>(join));
  EXPECT_EQ(std::get<SqlParseError>(join).code,
            SqlParseErrorCode::UnsupportedStatement);

  const auto comparison = parser.parse("SELECT * FROM users WHERE id > 2");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(comparison));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(comparison));
  ASSERT_TRUE(std::holds_alternative<WhereEquals>(select.where));
  EXPECT_EQ(std::get<WhereEquals>(select.where).comparison,
            ComparisonOperator::GreaterThan);
}

TEST(SqlParserTest, RejectsMismatchedInsertColumnAndValueCounts) {
  SqlParserImpl parser;
  const auto result = parser.parse("INSERT INTO users (id, name) VALUES (1)");

  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::SyntaxError);
}

TEST(SqlParserTest, RejectsUnsupportedLiteralTypes) {
  SqlParserImpl parser;
  const auto result = parser.parse("INSERT INTO users (active) VALUES (TRUE)");

  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::UnsupportedStatement);
}

TEST(SqlParserTest, ParsesEverySupportedWhereComparisonOperator) {
  SqlParserImpl parser;
  const std::vector<std::pair<std::string, ComparisonOperator>> cases{
      {"=", ComparisonOperator::Equal},
      {"!=", ComparisonOperator::NotEqual},
      {">", ComparisonOperator::GreaterThan},
      {"<", ComparisonOperator::LessThan},
      {">=", ComparisonOperator::GreaterThanOrEqual},
      {"<=", ComparisonOperator::LessThanOrEqual}};

  for (const auto &[token, expected] : cases) {
    const auto result =
        parser.parse("SELECT id FROM users WHERE id " + token + " 7;");
    ASSERT_TRUE(std::holds_alternative<SqlStatement>(result))
        << "operator: " << token;
    const auto &select =
        std::get<SelectStatement>(std::get<SqlStatement>(result));
    ASSERT_TRUE(std::holds_alternative<WhereEquals>(select.where));
    EXPECT_EQ(std::get<WhereEquals>(select.where).comparison, expected)
        << "operator: " << token;
  }
}

TEST(SqlParserTest, RejectsIncompleteWhereComparison) {
  SqlParserImpl parser;
  const auto result = parser.parse("SELECT * FROM users WHERE id >");
  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::SyntaxError);
}

} // namespace
} // namespace unidb::sql
