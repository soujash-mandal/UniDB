#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include <variant>

namespace unidb::sql {
namespace {

TEST(SqlParserIntegerExpressionTest, ParsesPrecedenceAndParentheses) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("SELECT id + 2 * (3 - 1) AS score FROM users");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(select.integerExpressions.size(), 1U);
  ASSERT_TRUE(select.integerExpressions[0].has_value());
  const auto &expression = *select.integerExpressions[0];
  ASSERT_EQ(expression.kind, IntegerExpression::Kind::Binary);
  EXPECT_EQ(expression.operation, IntegerBinaryOperator::Add);
  ASSERT_NE(expression.left, nullptr);
  ASSERT_NE(expression.right, nullptr);
  EXPECT_EQ(expression.left->kind, IntegerExpression::Kind::Column);
  EXPECT_EQ(expression.left->columnName, "id");
  EXPECT_EQ(expression.right->operation, IntegerBinaryOperator::Multiply);
  ASSERT_TRUE(select.columnAliases[0].has_value());
  EXPECT_EQ(*select.columnAliases[0], "score");
}

TEST(SqlParserIntegerExpressionTest, ParsesIntegerLiteralExpressions) {
  SqlParserImpl parser;
  const auto result = parser.parse("SELECT (8 / 3) - 2 FROM users");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_TRUE(select.integerExpressions[0].has_value());
  EXPECT_EQ(select.integerExpressions[0]->operation,
            IntegerBinaryOperator::Subtract);
}

TEST(SqlParserIntegerExpressionTest,
     ParsesUnaryPositiveAndNegativeOperators) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "SELECT +id, -(id + 1), --5, -9223372036854775808 FROM users");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  ASSERT_EQ(select.integerExpressions.size(), 4U);
  ASSERT_TRUE(select.integerExpressions[0].has_value());
  EXPECT_EQ(select.integerExpressions[0]->kind, IntegerExpression::Kind::Unary);
  EXPECT_EQ(select.integerExpressions[0]->unaryOperation,
            IntegerUnaryOperator::Positive);
  ASSERT_TRUE(select.integerExpressions[1].has_value());
  EXPECT_EQ(select.integerExpressions[1]->unaryOperation,
            IntegerUnaryOperator::Negative);
  ASSERT_TRUE(select.integerExpressions[2].has_value());
  EXPECT_EQ(select.integerExpressions[2]->kind, IntegerExpression::Kind::Unary);
  EXPECT_EQ(select.integerExpressions[3]->literal,
            std::numeric_limits<std::int64_t>::min());
}

TEST(SqlParserIntegerExpressionTest,
     RejectsUnaryOperatorOverflowAndMalformedInput) {
  SqlParserImpl parser;
  for (const auto *sql : {
           "SELECT + FROM users",
           "SELECT - FROM users",
           "SELECT +9223372036854775808 FROM users",
           "SELECT -9223372036854775809 FROM users",
       }) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
  }
}

TEST(SqlParserIntegerExpressionTest, RejectsMalformedAndOutOfRangeExpressions) {
  SqlParserImpl parser;
  for (const auto *sql : {
           "SELECT id + FROM users",
           "SELECT (id * 2 FROM users",
           "SELECT id / / 2 FROM users",
           "SELECT 9223372036854775808 FROM users",
       }) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
  }
}

} // namespace
} // namespace unidb::sql
