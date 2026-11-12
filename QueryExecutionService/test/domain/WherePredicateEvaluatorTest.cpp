#include "../../domain/predicate/WherePredicateEvaluator.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using QueryExecutionService::domain::compareSqlValues;
using QueryExecutionService::domain::compileWhereExpression;
using QueryExecutionService::domain::evaluateWhereExpression;
using QueryExecutionService::domain::matchesWhereExpression;
using unidb::sql::ColumnDefinition;
using unidb::sql::ComparisonOperator;
using unidb::sql::SqlType;
using unidb::sql::SqlTypeDescriptor;
using unidb::sql::SqlTypeId;
using unidb::sql::SqlValue;
using unidb::sql::WhereEquals;
using unidb::sql::WhereExpression;
} // namespace

TEST(WherePredicateEvaluatorTest, ComparesIntegerAndTextValues) {
  EXPECT_LT(
      compareSqlValues(SqlValue{std::int64_t{1}}, SqlValue{std::int64_t{2}}),
      0);
  EXPECT_EQ(compareSqlValues(SqlValue{std::string("same")},
                             SqlValue{std::string("same")}),
            0);
  EXPECT_GT(
      compareSqlValues(SqlValue{std::string("z")}, SqlValue{std::string("a")}),
      0);
}

TEST(WherePredicateEvaluatorTest,
     AppliesBlankPaddedComparisonOnlyToCharColumns) {
  const std::vector<ColumnDefinition> schema{
      {"fixed", SqlType::Text, SqlTypeDescriptor{SqlTypeId::Char, {8}}},
      {"bounded", SqlType::Text, SqlTypeDescriptor{SqlTypeId::Varchar, {8}}},
      {"plain", SqlType::Text, SqlTypeDescriptor{SqlTypeId::Text, {}}}};
  const std::vector<SqlType> types{SqlType::Text, SqlType::Text, SqlType::Text};
  const std::vector<SqlValue> row{std::string("hello   "),
                                  std::string("hello   "),
                                  std::string("hello   ")};

  const auto equals = [&](const std::string &column, const std::string &value) {
    WhereExpression expression;
    expression.predicate =
        WhereEquals{column, value, std::nullopt, {}, ComparisonOperator::Equal};
    return matchesWhereExpression(
        row, compileWhereExpression(expression, schema, types));
  };

  EXPECT_TRUE(equals("fixed", "hello"));
  EXPECT_FALSE(equals("bounded", "hello"));
  EXPECT_FALSE(equals("plain", "hello"));
  EXPECT_TRUE(equals("fixed", "hello   "));
  EXPECT_TRUE(equals("bounded", "hello   "));
}

TEST(WherePredicateEvaluatorTest,
     AppliesCharComparisonToOrderingMembershipAndRanges) {
  const std::vector<ColumnDefinition> schema{
      {"fixed", SqlType::Text, SqlTypeDescriptor{SqlTypeId::Char, {5}}}};
  const std::vector<SqlType> types{SqlType::Text};
  const std::vector<SqlValue> row{std::string("cat  ")};

  WhereExpression greater;
  greater.predicate = WhereEquals{"fixed",
                                  std::string("cat"),
                                  std::nullopt,
                                  {},
                                  ComparisonOperator::GreaterThan};
  EXPECT_FALSE(matchesWhereExpression(
      row, compileWhereExpression(greater, schema, types)));

  WhereExpression membership;
  membership.predicate = WhereEquals{"fixed",
                                     std::string("unused"),
                                     std::nullopt,
                                     {std::string("cat")},
                                     ComparisonOperator::In};
  EXPECT_TRUE(matchesWhereExpression(
      row, compileWhereExpression(membership, schema, types)));

  WhereExpression range;
  range.predicate = WhereEquals{"fixed",
                                std::string("cat"),
                                std::string("cat"),
                                {},
                                ComparisonOperator::Between};
  EXPECT_TRUE(matchesWhereExpression(
      row, compileWhereExpression(range, schema, types)));
}

TEST(WherePredicateEvaluatorTest,
     CharacterComparisonPreservesLeadingSpacesAndUtf8) {
  const std::vector<ColumnDefinition> schema{
      {"fixed", SqlType::Text, SqlTypeDescriptor{SqlTypeId::Char, {8}}},
      {"bounded", SqlType::Text, SqlTypeDescriptor{SqlTypeId::Varchar, {8}}}};
  const std::vector<SqlType> types{SqlType::Text, SqlType::Text};
  const std::vector<SqlValue> row{std::string(" é猫     "),
                                  std::string("é猫  ")};

  WhereExpression fixed;
  fixed.predicate = WhereEquals{"fixed",
                                std::string(" é猫"),
                                std::nullopt,
                                {},
                                ComparisonOperator::Equal};
  EXPECT_TRUE(matchesWhereExpression(
      row, compileWhereExpression(fixed, schema, types)));

  WhereExpression varchar;
  varchar.predicate = WhereEquals{"bounded",
                                  std::string("é猫"),
                                  std::nullopt,
                                  {},
                                  ComparisonOperator::Equal};
  EXPECT_FALSE(matchesWhereExpression(
      row, compileWhereExpression(varchar, schema, types)));
}

TEST(WherePredicateEvaluatorTest, RejectsValuesWithDifferentTypes) {
  EXPECT_THROW(
      compareSqlValues(SqlValue{std::int64_t{1}}, SqlValue{std::string("1")}),
      std::invalid_argument);
}

TEST(WherePredicateEvaluatorTest,
     EvaluatesComparisonMembershipRangeAndLikePredicates) {
  const std::vector<ColumnDefinition> schema{{"id", SqlType::Int},
                                             {"name", SqlType::Text}};
  const std::vector<SqlType> types{SqlType::Int, SqlType::Text};
  const std::vector<SqlValue> row{std::int64_t{3}, std::string("Alice")};

  WhereExpression greater;
  greater.predicate = WhereEquals{
      "id", std::int64_t{2}, std::nullopt, {}, ComparisonOperator::GreaterThan};
  EXPECT_TRUE(matchesWhereExpression(
      row, compileWhereExpression(greater, schema, types)));

  WhereExpression membership;
  membership.predicate = WhereEquals{"id",
                                     std::int64_t{0},
                                     std::nullopt,
                                     {std::int64_t{1}, std::int64_t{3}},
                                     ComparisonOperator::In};
  EXPECT_TRUE(matchesWhereExpression(
      row, compileWhereExpression(membership, schema, types)));

  WhereExpression range;
  range.predicate = WhereEquals{"id",
                                std::int64_t{2},
                                SqlValue{std::int64_t{4}},
                                {},
                                ComparisonOperator::Between};
  EXPECT_TRUE(matchesWhereExpression(
      row, compileWhereExpression(range, schema, types)));

  WhereExpression like;
  like.predicate = WhereEquals{
      "name", std::string("A%"), std::nullopt, {}, ComparisonOperator::Like};
  EXPECT_TRUE(
      matchesWhereExpression(row, compileWhereExpression(like, schema, types)));
}

TEST(WherePredicateEvaluatorTest, EvaluatesNestedAndOrExpressions) {
  const std::vector<ColumnDefinition> schema{{"id", SqlType::Int},
                                             {"name", SqlType::Text}};
  const std::vector<SqlType> types{SqlType::Int, SqlType::Text};
  const std::vector<SqlValue> row{std::int64_t{3}, std::string("Alice")};

  WhereExpression idMatch;
  idMatch.predicate = WhereEquals{
      "id", std::int64_t{3}, std::nullopt, {}, ComparisonOperator::Equal};
  WhereExpression nameMatch;
  nameMatch.predicate = WhereEquals{"name",
                                    std::string("Alice"),
                                    std::nullopt,
                                    {},
                                    ComparisonOperator::Equal};
  WhereExpression expression;
  expression.kind = WhereExpression::Kind::And;
  expression.children = {idMatch, nameMatch};
  EXPECT_TRUE(matchesWhereExpression(
      row, compileWhereExpression(expression, schema, types)));

  WhereExpression idDoesNotMatch;
  idDoesNotMatch.predicate = WhereEquals{
      "id", std::int64_t{99}, std::nullopt, {}, ComparisonOperator::Equal};
  expression.kind = WhereExpression::Kind::Or;
  expression.children = {idDoesNotMatch, nameMatch};
  EXPECT_TRUE(matchesWhereExpression(
      row, compileWhereExpression(expression, schema, types)));
}

TEST(WherePredicateEvaluatorTest,
     RejectsUnknownColumnsAndMismatchedTypesDuringCompilation) {
  const std::vector<ColumnDefinition> schema{{"id", SqlType::Int}};
  const std::vector<SqlType> types{SqlType::Int};
  WhereExpression unknown;
  unknown.predicate = WhereEquals{
      "missing", std::int64_t{1}, std::nullopt, {}, ComparisonOperator::Equal};
  EXPECT_THROW(compileWhereExpression(unknown, schema, types),
               std::invalid_argument);

  WhereExpression mismatch;
  mismatch.predicate = WhereEquals{
      "id", std::string("1"), std::nullopt, {}, ComparisonOperator::Equal};
  EXPECT_THROW(compileWhereExpression(mismatch, schema, types),
               std::invalid_argument);
}

TEST(WherePredicateEvaluatorTest,
     ComparesRealAndDoublePrecisionValuesIncludingPostgresNaNOrdering) {
  EXPECT_LT(compareSqlValues(SqlValue{1.25F}, SqlValue{2.5F}), 0);
  EXPECT_GT(compareSqlValues(SqlValue{2.5}, SqlValue{1.25}), 0);
  EXPECT_EQ(compareSqlValues(SqlValue{-0.0F}, SqlValue{0.0F}), 0);
  EXPECT_EQ(compareSqlValues(SqlValue{-0.0}, SqlValue{0.0}), 0);

  const float realNaN = std::numeric_limits<float>::quiet_NaN();
  const double doubleNaN = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(compareSqlValues(SqlValue{realNaN}, SqlValue{realNaN}), 0);
  EXPECT_EQ(compareSqlValues(SqlValue{doubleNaN}, SqlValue{doubleNaN}), 0);
  EXPECT_GT(compareSqlValues(SqlValue{realNaN}, SqlValue{1.0F}), 0);
  EXPECT_LT(compareSqlValues(SqlValue{1.0}, SqlValue{doubleNaN}), 0);
  EXPECT_THROW(compareSqlValues(SqlValue{1.0F}, SqlValue{1.0}),
               std::invalid_argument);
}

TEST(WherePredicateEvaluatorTest,
     CompilesAndEvaluatesRealAndDoublePrecisionWhereComparisons) {
  const std::vector<ColumnDefinition> schema{
      {"approximate", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Real, {}}},
      {"precise", SqlType::Int,
       SqlTypeDescriptor{SqlTypeId::DoublePrecision, {}}}};
  const std::vector<SqlType> types{SqlType::Int, SqlType::Int};
  const std::vector<SqlValue> row{1.5F, 2.5};

  WhereExpression realGreater;
  realGreater.predicate = WhereEquals{"approximate",
                                      SqlValue{1.0},
                                      std::nullopt,
                                      {},
                                      ComparisonOperator::GreaterThan};
  EXPECT_TRUE(matchesWhereExpression(
      row, compileWhereExpression(realGreater, schema, types)));

  WhereExpression doubleEqual;
  doubleEqual.predicate = WhereEquals{
      "precise", SqlValue{2.5}, std::nullopt, {}, ComparisonOperator::Equal};
  EXPECT_TRUE(matchesWhereExpression(
      row, compileWhereExpression(doubleEqual, schema, types)));
}

TEST(WherePredicateEvaluatorTest, AppliesIsNullAndIsNotNullPredicates) {
  const std::vector<ColumnDefinition> schema{{"id", SqlType::Int}};
  const std::vector<SqlType> types{SqlType::Int};
  const std::vector<SqlValue> nullRow{unidb::sql::SqlNullValue{}};
  const std::vector<SqlValue> valueRow{std::int64_t{7}};

  WhereExpression isNull;
  isNull.predicate.columnName = "id";
  isNull.predicate.comparison = ComparisonOperator::IsNull;
  const auto compiledNull = compileWhereExpression(isNull, schema, types);
  EXPECT_EQ(evaluateWhereExpression(nullRow, compiledNull),
            QueryExecutionService::domain::SqlTruthValue::True);
  EXPECT_EQ(evaluateWhereExpression(valueRow, compiledNull),
            QueryExecutionService::domain::SqlTruthValue::False);

  WhereExpression isNotNull = isNull;
  isNotNull.predicate.comparison = ComparisonOperator::IsNotNull;
  const auto compiledNotNull = compileWhereExpression(isNotNull, schema, types);
  EXPECT_EQ(evaluateWhereExpression(nullRow, compiledNotNull),
            QueryExecutionService::domain::SqlTruthValue::False);
  EXPECT_EQ(evaluateWhereExpression(valueRow, compiledNotNull),
            QueryExecutionService::domain::SqlTruthValue::True);
}

TEST(WherePredicateEvaluatorTest, ComparisonsWithNullProduceUnknown) {
  const std::vector<ColumnDefinition> schema{{"id", SqlType::Int}};
  const std::vector<SqlType> types{SqlType::Int};
  const std::vector<SqlValue> row{unidb::sql::SqlNullValue{}};
  WhereExpression equals;
  equals.predicate = WhereEquals{
      "id", std::int64_t{7}, std::nullopt, {}, ComparisonOperator::Equal};
  const auto compiled = compileWhereExpression(equals, schema, types);
  EXPECT_EQ(evaluateWhereExpression(row, compiled),
            QueryExecutionService::domain::SqlTruthValue::Unknown);
  EXPECT_FALSE(matchesWhereExpression(row, compiled));
}

TEST(WherePredicateEvaluatorTest, AppliesSqlThreeValuedAndOrTruthTables) {
  const std::vector<ColumnDefinition> schema{{"id", SqlType::Int}};
  const std::vector<SqlType> types{SqlType::Int};
  const std::vector<SqlValue> row{unidb::sql::SqlNullValue{}};
  WhereExpression unknown;
  unknown.predicate = WhereEquals{
      "id", std::int64_t{1}, std::nullopt, {}, ComparisonOperator::Equal};
  WhereExpression falsePredicate;
  falsePredicate.predicate = WhereEquals{
      "id", unidb::sql::SqlNullValue{}, std::nullopt, {},
      ComparisonOperator::IsNotNull};
  WhereExpression truePredicate;
  truePredicate.predicate = WhereEquals{"id",
                                        unidb::sql::SqlNullValue{},
                                        std::nullopt,
                                        {},
                                        ComparisonOperator::IsNull};

  WhereExpression conjunction;
  conjunction.kind = WhereExpression::Kind::And;
  conjunction.children = {unknown, falsePredicate};
  EXPECT_EQ(evaluateWhereExpression(
                row, compileWhereExpression(conjunction, schema, types)),
            QueryExecutionService::domain::SqlTruthValue::False);

  WhereExpression disjunction;
  disjunction.kind = WhereExpression::Kind::Or;
  disjunction.children = {unknown, truePredicate};
  EXPECT_EQ(evaluateWhereExpression(
                row, compileWhereExpression(disjunction, schema, types)),
            QueryExecutionService::domain::SqlTruthValue::True);

  disjunction.children = {unknown, falsePredicate};
  EXPECT_EQ(evaluateWhereExpression(
                row, compileWhereExpression(disjunction, schema, types)),
            QueryExecutionService::domain::SqlTruthValue::Unknown);
}
