#include "../SqlParserImpl.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace unidb::sql {
namespace {

TEST(SqlParserTest, ParsesSelectAllAndOptionalEqualityPredicate) {
  SqlParserImpl parser;
  const auto result = parser.parse("SELECT * FROM users WHERE id = 7;");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  EXPECT_TRUE(select.selectAll);
  EXPECT_EQ(select.tableName, "users");
  ASSERT_TRUE(std::holds_alternative<WhereExpression>(select.where));
  const auto &where = std::get<WhereExpression>(select.where);
  EXPECT_EQ(where.kind, WhereExpression::Kind::Predicate);
  EXPECT_EQ(where.predicate.columnName, "id");
  EXPECT_EQ(std::get<std::int64_t>(where.predicate.value), 7);
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
    ASSERT_TRUE(std::holds_alternative<WhereExpression>(select.where));
    EXPECT_EQ(std::get<WhereExpression>(select.where).predicate.comparison,
              expected)
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

TEST(SqlParserTest, ParsesAndBeforeOrPrecedence) {
  SqlParserImpl parser;
  const auto result = parser.parse(
      "SELECT id FROM users WHERE id = 1 OR id = 2 AND name = 'B'");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  const auto &where = std::get<WhereExpression>(select.where);
  ASSERT_EQ(where.kind, WhereExpression::Kind::Or);
  ASSERT_EQ(where.children.size(), 2U);
  EXPECT_EQ(where.children[0].kind, WhereExpression::Kind::Predicate);
  ASSERT_EQ(where.children[1].kind, WhereExpression::Kind::And);
  ASSERT_EQ(where.children[1].children.size(), 2U);
}

TEST(SqlParserTest, RejectsDanglingAndOrAndRepeatedConnectors) {
  SqlParserImpl parser;
  for (const auto *sql : {"SELECT * FROM users WHERE id = 1 AND",
                          "SELECT * FROM users WHERE id = 1 OR",
                          "SELECT * FROM users WHERE id = 1 AND OR id = 2"}) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
    EXPECT_EQ(std::get<SqlParseError>(result).code,
              SqlParseErrorCode::SyntaxError)
        << sql;
  }
}

TEST(SqlParserTest, ParsesParenthesizedWhereGroupsAndNestedPrecedence) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("SELECT id FROM users WHERE (id = 1 OR id = 2) AND "
                   "(name = 'A' OR (id > 3 AND name = 'B'))");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  const auto &where = std::get<WhereExpression>(select.where);
  ASSERT_EQ(where.kind, WhereExpression::Kind::And);
  ASSERT_EQ(where.children.size(), 2U);
  EXPECT_EQ(where.children[0].kind, WhereExpression::Kind::Or);
  ASSERT_EQ(where.children[0].children.size(), 2U);
  EXPECT_EQ(where.children[1].kind, WhereExpression::Kind::Or);
  ASSERT_EQ(where.children[1].children.size(), 2U);
  EXPECT_EQ(where.children[1].children[1].kind, WhereExpression::Kind::And);
}

TEST(SqlParserTest, ParenthesesOverrideAndBeforeOrPrecedence) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("SELECT id FROM users WHERE (id = 1 OR id = 2) AND "
                   "name = 'B'");

  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  const auto &where = std::get<WhereExpression>(select.where);
  ASSERT_EQ(where.kind, WhereExpression::Kind::And);
  ASSERT_EQ(where.children.size(), 2U);
  EXPECT_EQ(where.children[0].kind, WhereExpression::Kind::Or);
  EXPECT_EQ(where.children[1].kind, WhereExpression::Kind::Predicate);
}

TEST(SqlParserTest, RejectsMalformedParenthesizedWhereExpressions) {
  SqlParserImpl parser;
  for (const auto *sql :
       {"SELECT * FROM users WHERE (id = 1 OR name = 'A'",
        "SELECT * FROM users WHERE id = 1)", "SELECT * FROM users WHERE ()",
        "SELECT * FROM users WHERE (id = 1 AND)",
        "SELECT * FROM users WHERE (OR id = 1)",
        "SELECT * FROM users WHERE ((id = 1)"}) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
    EXPECT_EQ(std::get<SqlParseError>(result).code,
              SqlParseErrorCode::SyntaxError)
        << sql;
  }
}

TEST(SqlParserTest, RejectsExcessivelyDeepWhereNesting) {
  SqlParserImpl parser;
  std::string sql = "SELECT * FROM users WHERE ";
  for (int i = 0; i < 129; ++i) {
    sql += "(";
  }
  sql += "id = 1";
  for (int i = 0; i < 129; ++i) {
    sql += ")";
  }

  const auto result = parser.parse(sql);
  ASSERT_TRUE(std::holds_alternative<SqlParseError>(result));
  EXPECT_EQ(std::get<SqlParseError>(result).code,
            SqlParseErrorCode::SyntaxError);
}

TEST(SqlParserTest, ParsesInAndNotInLiteralLists) {
  SqlParserImpl parser;
  const auto inResult =
      parser.parse("SELECT id FROM users WHERE id IN (1, 2, -3)");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(inResult));
  const auto &inSelect =
      std::get<SelectStatement>(std::get<SqlStatement>(inResult));
  const auto &inPredicate = std::get<WhereExpression>(inSelect.where).predicate;
  EXPECT_EQ(inPredicate.comparison, ComparisonOperator::In);
  ASSERT_EQ(inPredicate.values.size(), 3U);
  EXPECT_EQ(std::get<std::int64_t>(inPredicate.values[0]), 1);
  EXPECT_EQ(std::get<std::int64_t>(inPredicate.values[1]), 2);
  EXPECT_EQ(std::get<std::int64_t>(inPredicate.values[2]), -3);

  const auto notInResult =
      parser.parse("SELECT name FROM users WHERE name NOT IN ('Ada', 'Grace')");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(notInResult));
  const auto &notInSelect =
      std::get<SelectStatement>(std::get<SqlStatement>(notInResult));
  const auto &notInPredicate =
      std::get<WhereExpression>(notInSelect.where).predicate;
  EXPECT_EQ(notInPredicate.comparison, ComparisonOperator::NotIn);
  ASSERT_EQ(notInPredicate.values.size(), 2U);
  EXPECT_EQ(std::get<std::string>(notInPredicate.values[0]), "Ada");
  EXPECT_EQ(std::get<std::string>(notInPredicate.values[1]), "Grace");
}

TEST(SqlParserTest, ParsesBetweenAndNotBetweenRangePredicates) {
  SqlParserImpl parser;
  const auto betweenResult =
      parser.parse("SELECT id FROM users WHERE id BETWEEN -2 AND 10");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(betweenResult));
  const auto &betweenSelect =
      std::get<SelectStatement>(std::get<SqlStatement>(betweenResult));
  const auto &between =
      std::get<WhereExpression>(betweenSelect.where).predicate;
  EXPECT_EQ(between.comparison, ComparisonOperator::Between);
  EXPECT_EQ(std::get<std::int64_t>(between.value), -2);
  ASSERT_TRUE(between.upperBound.has_value());
  EXPECT_EQ(std::get<std::int64_t>(*between.upperBound), 10);

  const auto notBetweenResult = parser.parse(
      "SELECT name FROM users WHERE name NOT BETWEEN 'Ada' AND 'Grace'");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(notBetweenResult));
  const auto &notBetweenSelect =
      std::get<SelectStatement>(std::get<SqlStatement>(notBetweenResult));
  const auto &notBetween =
      std::get<WhereExpression>(notBetweenSelect.where).predicate;
  EXPECT_EQ(notBetween.comparison, ComparisonOperator::NotBetween);
  EXPECT_EQ(std::get<std::string>(notBetween.value), "Ada");
  ASSERT_TRUE(notBetween.upperBound.has_value());
  EXPECT_EQ(std::get<std::string>(*notBetween.upperBound), "Grace");
}

TEST(SqlParserTest, RejectsMalformedBetweenPredicates) {
  SqlParserImpl parser;
  for (const auto *sql : {"SELECT * FROM users WHERE id BETWEEN 1",
                          "SELECT * FROM users WHERE id BETWEEN AND 3",
                          "SELECT * FROM users WHERE id BETWEEN 1 AND",
                          "SELECT * FROM users WHERE id NOT BETWEEN 1",
                          "SELECT * FROM users WHERE id NOT IN 1",
                          "SELECT * FROM users WHERE id BETWEEN 1 OR id = 2"}) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
    EXPECT_EQ(std::get<SqlParseError>(result).code,
              SqlParseErrorCode::SyntaxError)
        << sql;
  }
}

TEST(SqlParserTest, RejectsMalformedInLists) {
  SqlParserImpl parser;
  for (const auto *sql : {"SELECT * FROM users WHERE id IN ()",
                          "SELECT * FROM users WHERE id IN (1,)",
                          "SELECT * FROM users WHERE id IN (1, 2",
                          "SELECT * FROM users WHERE id IN 1",
                          "SELECT * FROM users WHERE id NOT 1",
                          "SELECT * FROM users WHERE id NOT IN (1, 2))",
                          "SELECT * FROM users WHERE id IN (1) AND"}) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
    EXPECT_EQ(std::get<SqlParseError>(result).code,
              SqlParseErrorCode::SyntaxError)
        << sql;
  }
}

TEST(SqlParserTest, ParsesLikeAndNotLikePatterns) {
  SqlParserImpl parser;
  const auto likeResult =
      parser.parse("SELECT name FROM users WHERE name LIKE 'A%_'");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(likeResult));
  const auto &likeSelect =
      std::get<SelectStatement>(std::get<SqlStatement>(likeResult));
  const auto &like = std::get<WhereExpression>(likeSelect.where).predicate;
  EXPECT_EQ(like.comparison, ComparisonOperator::Like);
  EXPECT_EQ(std::get<std::string>(like.value), "A%_");

  const auto notLikeResult =
      parser.parse("SELECT name FROM users WHERE name NOT LIKE '%son'");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(notLikeResult));
  const auto &notLikeSelect =
      std::get<SelectStatement>(std::get<SqlStatement>(notLikeResult));
  const auto &notLike =
      std::get<WhereExpression>(notLikeSelect.where).predicate;
  EXPECT_EQ(notLike.comparison, ComparisonOperator::NotLike);
  EXPECT_EQ(std::get<std::string>(notLike.value), "%son");
}

TEST(SqlParserTest, RejectsMalformedLikePredicates) {
  SqlParserImpl parser;
  const std::vector<const char *> queries{
      "SELECT * FROM users WHERE name LIKE",
      "SELECT * FROM users WHERE name NOT LIKE",
      "SELECT * FROM users WHERE name NOT LIKE 'A%' AND"};
  for (const auto *sql : queries) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlParseError>(result)) << sql;
    EXPECT_EQ(std::get<SqlParseError>(result).code,
              SqlParseErrorCode::SyntaxError)
        << sql;
  }
}

} // namespace
} // namespace unidb::sql

namespace unidb::sql {
namespace {

TEST(SqlParserWhereTest, ParsesIsNullAndIsNotNullPredicates) {
  SqlParserImpl parser;
  for (const auto &[sql, expected] :
       std::vector<std::pair<std::string, ComparisonOperator>>{
           {"SELECT id FROM people WHERE id IS NULL",
            ComparisonOperator::IsNull},
           {"SELECT id FROM people WHERE id IS NOT NULL",
            ComparisonOperator::IsNotNull}}) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<SqlStatement>(result)) << sql;
    const auto &select =
        std::get<SelectStatement>(std::get<SqlStatement>(result));
    ASSERT_TRUE(std::holds_alternative<WhereExpression>(select.where));
    EXPECT_EQ(std::get<WhereExpression>(select.where).predicate.comparison,
              expected);
  }
}

TEST(SqlParserWhereTest, ParsesNullComparisonLiteral) {
  SqlParserImpl parser;
  const auto result =
      parser.parse("SELECT id FROM people WHERE id = NULL");
  ASSERT_TRUE(std::holds_alternative<SqlStatement>(result));
  const auto &select =
      std::get<SelectStatement>(std::get<SqlStatement>(result));
  const auto &predicate = std::get<WhereExpression>(select.where).predicate;
  EXPECT_TRUE(std::holds_alternative<SqlNullValue>(predicate.value));
}

} // namespace
} // namespace unidb::sql
