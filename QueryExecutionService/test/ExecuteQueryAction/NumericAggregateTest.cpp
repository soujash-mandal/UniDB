#include "../../domain/types/SqlNumericSemantics.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

#include <string>
#include <variant>

namespace {
using QueryExecutionService::domain::formatSqlNumericValue;
using unidb::sql::SqlNumericValue;
using unidb::sql::SqlType;
using unidb::sql::SqlTypeDescriptor;
using unidb::sql::SqlTypeId;

FakeQesRdbServicePort makeNumericAggregatePort() {
  FakeQesRdbServicePort port;
  port.schema = {
      {"amount", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Numeric, {}}}};
  return port;
}

void insertNumericRows(FakeQesRdbServicePort &port,
                       unidb::sql::SqlParserImpl &parser,
                       const std::string &values) {
  auto action = makeQesAction(port, parser);
  const auto inserted =
      action.execute("INSERT INTO decimals (amount) VALUES " + values);
  EXPECT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          inserted));
  port.rows = port.insertedRows;
}
} // namespace

TEST(ExecuteQueryActionNumericAggregateTest,
     CalculatesExactUngroupedSumAndAverage) {
  auto port = makeNumericAggregatePort();
  unidb::sql::SqlParserImpl parser;
  insertNumericRows(port, parser, "(0.1), (0.2)");

  auto action = makeQesAction(port, parser);
  const auto sum = action.execute("SELECT SUM(amount) FROM decimals");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(sum));
  const auto &sumRows =
      std::get<QueryExecutionService::QueryExecutionResult>(sum).rows;
  ASSERT_EQ(sumRows.size(), 1U);
  EXPECT_EQ(formatSqlNumericValue(std::get<SqlNumericValue>(sumRows[0][0])),
            "0.3");

  const auto average = action.execute("SELECT AVG(amount) FROM decimals");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          average));
  const auto &averageRows =
      std::get<QueryExecutionService::QueryExecutionResult>(average).rows;
  ASSERT_EQ(averageRows.size(), 1U);
  EXPECT_EQ(formatSqlNumericValue(std::get<SqlNumericValue>(averageRows[0][0])),
            "0.15");
}

TEST(ExecuteQueryActionNumericAggregateTest,
     GroupsExactNumericSumsAndOrdersByAggregateResult) {
  FakeQesRdbServicePort port;
  port.schema = {
      {"department", SqlType::Text, SqlTypeDescriptor{SqlTypeId::Text, {}}},
      {"amount", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Numeric, {}}}};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto inserted =
      action.execute("INSERT INTO decimals (department, amount) VALUES "
                     "('engineering', 0.1), ('engineering', 0.2), "
                     "('sales', 0.5), ('sales', 0.1), ('support', 0.2)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          inserted));
  port.rows = port.insertedRows;

  const auto selected =
      action.execute("SELECT department, SUM(amount) AS total FROM decimals "
                     "GROUP BY department ORDER BY total DESC");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          selected));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(selected);
  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "sales");
  EXPECT_EQ(formatSqlNumericValue(std::get<SqlNumericValue>(result.rows[0][1])),
            "0.6");
  EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "engineering");
  EXPECT_EQ(formatSqlNumericValue(std::get<SqlNumericValue>(result.rows[1][1])),
            "0.3");
  EXPECT_EQ(std::get<std::string>(result.rows[2][0]), "support");
  EXPECT_EQ(formatSqlNumericValue(std::get<SqlNumericValue>(result.rows[2][1])),
            "0.2");
  ASSERT_EQ(result.columns.size(), 2U);
  ASSERT_TRUE(result.columns[1].declaredType.has_value());
  EXPECT_EQ(result.columns[1].declaredType->id, SqlTypeId::Numeric);
}

TEST(ExecuteQueryActionNumericAggregateTest,
     GroupsNumericallyEqualValuesIntoOneGroup) {
  auto port = makeNumericAggregatePort();
  unidb::sql::SqlParserImpl parser;
  insertNumericRows(port, parser, "(1.0), (1.00), (2.0), (2.00)");

  auto action = makeQesAction(port, parser);
  const auto selected = action.execute("SELECT amount, COUNT(*) FROM decimals "
                                       "GROUP BY amount ORDER BY amount ASC");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          selected));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(selected).rows;
  ASSERT_EQ(rows.size(), 2U);
  EXPECT_EQ(formatSqlNumericValue(std::get<SqlNumericValue>(rows[0][0])), "1");
  EXPECT_EQ(std::get<std::int64_t>(rows[0][1]), 2);
  EXPECT_EQ(formatSqlNumericValue(std::get<SqlNumericValue>(rows[1][0])), "2");
  EXPECT_EQ(std::get<std::int64_t>(rows[1][1]), 2);
}

TEST(ExecuteQueryActionNumericAggregateTest,
     FiltersGroupedNumericAggregatesWithExactHavingComparison) {
  FakeQesRdbServicePort port;
  port.schema = {
      {"department", SqlType::Text, SqlTypeDescriptor{SqlTypeId::Text, {}}},
      {"amount", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Numeric, {}}}};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto inserted =
      action.execute("INSERT INTO decimals (department, amount) VALUES "
                     "('engineering', 0.1), ('engineering', 0.2), "
                     "('sales', 0.5), ('sales', 0.1), ('support', 0.2)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          inserted));
  port.rows = port.insertedRows;

  const auto selected =
      action.execute("SELECT department, SUM(amount) AS total FROM decimals "
                     "GROUP BY department HAVING SUM(amount) > 0.3");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          selected));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(selected);
  ASSERT_EQ(result.rows.size(), 1U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "sales");
  EXPECT_EQ(formatSqlNumericValue(std::get<SqlNumericValue>(result.rows[0][1])),
            "0.6");
}
