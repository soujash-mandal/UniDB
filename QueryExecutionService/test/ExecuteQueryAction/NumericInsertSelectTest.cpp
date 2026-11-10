#include "../../domain/codec/SqlTupleCodec.h"
#include "../../domain/types/SqlNumericSemantics.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

#include <string>
#include <variant>

namespace {
using unidb::sql::SqlNumericValue;
using unidb::sql::SqlType;
using unidb::sql::SqlTypeDescriptor;
using unidb::sql::SqlTypeId;

FakeQesRdbServicePort makeNumericPort() {
  FakeQesRdbServicePort port;
  port.schema = {
      {"amount", SqlType::Int, SqlTypeDescriptor{SqlTypeId::Numeric, {}}}};
  return port;
}
} // namespace

TEST(ExecuteQueryActionNumericTest,
     InsertsSelectsAndFiltersExactDecimalValues) {
  auto port = makeNumericPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto create = action.execute("CREATE TABLE decimals (amount NUMERIC)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          create));
  ASSERT_EQ(port.createdColumns.size(), 1U);
  EXPECT_EQ(port.createdColumns[0].declaredType->id, SqlTypeId::Numeric);

  const auto inserted = action.execute(
      "INSERT INTO decimals (amount) VALUES "
      "(123456789012345678901234567890.123456789), (-0.0000000012)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          inserted));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionResult>(inserted)
                .affectedRows,
            2U);
  ASSERT_EQ(port.insertedRows.size(), 2U);

  const auto first = SqlTupleCodec::decode(port.schema, port.insertedRows[0]);
  const auto second = SqlTupleCodec::decode(port.schema, port.insertedRows[1]);
  EXPECT_EQ(
      std::get<SqlNumericValue>(first[0]),
      (SqlNumericValue{false, "123456789012345678901234567890123456789", 9}));
  EXPECT_EQ(std::get<SqlNumericValue>(second[0]),
            (SqlNumericValue{true, "12", 10}));

  port.rows = port.insertedRows;
  const auto selected =
      action.execute("SELECT amount FROM decimals ORDER BY amount");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          selected));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(selected).rows;
  ASSERT_EQ(rows.size(), 2U);
  EXPECT_EQ(std::get<SqlNumericValue>(rows[0][0]),
            (SqlNumericValue{true, "12", 10}));
  EXPECT_EQ(
      std::get<SqlNumericValue>(rows[1][0]),
      (SqlNumericValue{false, "123456789012345678901234567890123456789", 9}));

  const auto filtered =
      action.execute("SELECT amount FROM decimals "
                     "WHERE amount > 123456789012345678901234567890.123456788");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          filtered));
  const auto &filteredRows =
      std::get<QueryExecutionService::QueryExecutionResult>(filtered).rows;
  ASSERT_EQ(filteredRows.size(), 1U);
  EXPECT_EQ(std::get<SqlNumericValue>(filteredRows[0][0]),
            std::get<SqlNumericValue>(first[0]));
}

TEST(ExecuteQueryActionNumericTest, CoercesIntegerLiteralIntoNumericColumn) {
  auto port = makeNumericPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("INSERT INTO decimals (amount) VALUES (42)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  ASSERT_EQ(port.insertedRows.size(), 1U);
  const auto decoded = SqlTupleCodec::decode(port.schema, port.insertedRows[0]);
  EXPECT_EQ(std::get<SqlNumericValue>(decoded[0]),
            (SqlNumericValue{false, "42", 0}));
}

TEST(ExecuteQueryActionNumericTest,
     ConvertsExactDecimalLiteralsOnlyAfterResolvingFloatingColumnTypes) {
  FakeQesRdbServicePort port;
  port.schema = {
      {"approximate", unidb::sql::SqlType::Int,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Real, {}}},
      {"precise", unidb::sql::SqlType::Int,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::DoublePrecision,
                                     {}}}};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto inserted = action.execute(
      "INSERT INTO measurements (approximate, precise) VALUES (1.25, 2.125)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          inserted));
  const auto decoded = SqlTupleCodec::decode(port.schema, port.insertedRows[0]);
  EXPECT_FLOAT_EQ(std::get<float>(decoded[0]), 1.25F);
  EXPECT_DOUBLE_EQ(std::get<double>(decoded[1]), 2.125);

  port.rows = port.insertedRows;
  const auto filtered = action.execute("SELECT approximate FROM measurements "
                                       "WHERE approximate > 1.2");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          filtered));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(filtered).rows;
  ASSERT_EQ(rows.size(), 1U);
  EXPECT_FLOAT_EQ(std::get<float>(rows[0][0]), 1.25F);
}

TEST(ExecuteQueryActionNumericTest,
     AppliesDeclaredPrecisionAndScaleWithHalfAwayFromZeroRounding) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlTypeModifier modifier;
  modifier.precision = 5;
  modifier.scale = 2;
  port.schema = {{"amount", SqlType::Int,
                  SqlTypeDescriptor{SqlTypeId::Numeric, modifier}}};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto inserted = action.execute(
      "INSERT INTO decimals (amount) VALUES (1.235), (-1.235), (12.3)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          inserted));
  ASSERT_EQ(port.insertedRows.size(), 3U);

  const auto positive =
      SqlTupleCodec::decode(port.schema, port.insertedRows[0]);
  const auto negative =
      SqlTupleCodec::decode(port.schema, port.insertedRows[1]);
  const auto padded = SqlTupleCodec::decode(port.schema, port.insertedRows[2]);
  EXPECT_EQ(std::get<SqlNumericValue>(positive[0]),
            (SqlNumericValue{false, "124", 2}));
  EXPECT_EQ(std::get<SqlNumericValue>(negative[0]),
            (SqlNumericValue{true, "124", 2}));
  EXPECT_EQ(std::get<SqlNumericValue>(padded[0]),
            (SqlNumericValue{false, "123", 1}));

  const auto overflow =
      action.execute("INSERT INTO decimals (amount) VALUES (999.995)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          overflow));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(overflow)
                .message.find("NUMERIC field overflow"),
            std::string::npos);
}

TEST(ExecuteQueryActionNumericTest,
     SupportsNegativeScaleRoundingAndPrecisionOverflow) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlTypeModifier modifier;
  modifier.precision = 3;
  modifier.scale = -1;
  port.schema = {{"amount", SqlType::Int,
                  SqlTypeDescriptor{SqlTypeId::Numeric, modifier}}};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto inserted = action.execute(
      "INSERT INTO decimals (amount) VALUES (125), (-125), (1234)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          inserted));
  ASSERT_EQ(port.insertedRows.size(), 3U);

  const auto positive =
      SqlTupleCodec::decode(port.schema, port.insertedRows[0]);
  const auto negative =
      SqlTupleCodec::decode(port.schema, port.insertedRows[1]);
  const auto large = SqlTupleCodec::decode(port.schema, port.insertedRows[2]);
  EXPECT_EQ(std::get<SqlNumericValue>(positive[0]),
            (SqlNumericValue{false, "13", -1}));
  EXPECT_EQ(std::get<SqlNumericValue>(negative[0]),
            (SqlNumericValue{true, "13", -1}));
  EXPECT_EQ(std::get<SqlNumericValue>(large[0]),
            (SqlNumericValue{false, "123", -1}));

  const auto overflow =
      action.execute("INSERT INTO decimals (amount) VALUES (9995)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          overflow));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(overflow)
                .message.find("NUMERIC field overflow"),
            std::string::npos);
}

TEST(ExecuteQueryActionNumericTest, EvaluatesExactNumericArithmeticInSelect) {
  auto port = makeNumericPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto inserted =
      action.execute("INSERT INTO decimals (amount) VALUES (0.1)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          inserted));
  port.rows = port.insertedRows;

  const auto selected = action.execute(
      "SELECT amount + 0.2, amount - 0.2, amount * 3, amount / 4 "
      "FROM decimals");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          selected));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(selected).rows;
  ASSERT_EQ(rows.size(), 1U);
  ASSERT_EQ(rows[0].size(), 4U);
  EXPECT_EQ(QueryExecutionService::domain::formatSqlNumericValue(
                std::get<SqlNumericValue>(rows[0][0])),
            "0.3");
  EXPECT_EQ(QueryExecutionService::domain::formatSqlNumericValue(
                std::get<SqlNumericValue>(rows[0][1])),
            "-0.1");
  EXPECT_EQ(QueryExecutionService::domain::formatSqlNumericValue(
                std::get<SqlNumericValue>(rows[0][2])),
            "0.3");
  EXPECT_EQ(QueryExecutionService::domain::formatSqlNumericValue(
                std::get<SqlNumericValue>(rows[0][3])),
            "0.025");
}

TEST(ExecuteQueryActionNumericTest, EvaluatesMixedIntegerAndNumericArithmetic) {
  auto port = makeNumericPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto inserted =
      action.execute("INSERT INTO decimals (amount) VALUES (1.25)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          inserted));
  port.rows = port.insertedRows;

  const auto selected =
      action.execute("SELECT amount + 2, 2 * amount FROM decimals");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          selected));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(selected).rows;
  ASSERT_EQ(rows.size(), 1U);
  EXPECT_EQ(QueryExecutionService::domain::formatSqlNumericValue(
                std::get<SqlNumericValue>(rows[0][0])),
            "3.25");
  EXPECT_EQ(QueryExecutionService::domain::formatSqlNumericValue(
                std::get<SqlNumericValue>(rows[0][1])),
            "2.5");
}

TEST(ExecuteQueryActionNumericTest, ReportsNumericDivisionByZero) {
  auto port = makeNumericPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto inserted =
      action.execute("INSERT INTO decimals (amount) VALUES (1.25)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          inserted));
  port.rows = port.insertedRows;

  const auto outcome = action.execute("SELECT amount / 0.0 FROM decimals");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("Division by zero"),
            std::string::npos);
}

TEST(ExecuteQueryActionNumericTest,
     SupportsDistinctExactNumericArithmeticExpressions) {
  auto port = makeNumericPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto inserted = action.execute(
      "INSERT INTO decimals (amount) VALUES (0.1), (0.1), (0.2), (0.3)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          inserted));
  port.rows = port.insertedRows;

  const auto selected =
      action.execute("SELECT DISTINCT amount + 0.1 FROM decimals");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          selected));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(selected).rows;
  ASSERT_EQ(rows.size(), 3U);
  EXPECT_EQ(QueryExecutionService::domain::formatSqlNumericValue(
                std::get<SqlNumericValue>(rows[0][0])),
            "0.2");
  EXPECT_EQ(QueryExecutionService::domain::formatSqlNumericValue(
                std::get<SqlNumericValue>(rows[1][0])),
            "0.3");
  EXPECT_EQ(QueryExecutionService::domain::formatSqlNumericValue(
                std::get<SqlNumericValue>(rows[2][0])),
            "0.4");
}

TEST(ExecuteQueryActionNumericTest,
     SupportsExactNumericInNotInBetweenAndNotBetweenPredicates) {
  auto port = makeNumericPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto inserted =
      action.execute("INSERT INTO decimals (amount) VALUES "
                     "(0.1), (0.2), (0.3), (0.3000000000000000000001), (1)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          inserted));
  port.rows = port.insertedRows;

  const auto selectValues = [&](const std::string &predicate) {
    const auto outcome =
        action.execute("SELECT amount FROM decimals WHERE " + predicate);
    EXPECT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            outcome))
        << predicate;
    if (!std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            outcome)) {
      return std::vector<std::string>{};
    }
    const auto &rows =
        std::get<QueryExecutionService::QueryExecutionResult>(outcome).rows;
    std::vector<std::string> values;
    for (const auto &row : rows) {
      values.push_back(QueryExecutionService::domain::formatSqlNumericValue(
          std::get<SqlNumericValue>(row[0])));
    }
    return values;
  };

  EXPECT_EQ(selectValues("amount IN (0.1, 0.3000000000000000000001, 1)"),
            (std::vector<std::string>{"0.1", "0.3000000000000000000001", "1"}));
  EXPECT_EQ(selectValues("amount NOT IN (0.1, 0.3, 1)"),
            (std::vector<std::string>{"0.2", "0.3000000000000000000001"}));
  EXPECT_EQ(selectValues("amount BETWEEN 0.1 AND 0.3"),
            (std::vector<std::string>{"0.1", "0.2", "0.3"}));
  EXPECT_EQ(selectValues("amount NOT BETWEEN 0.1 AND 0.3"),
            (std::vector<std::string>{"0.3000000000000000000001", "1"}));

  // Integer literals are normalized to exact NUMERIC values for these
  // predicates, just as they are for ordinary NUMERIC comparisons.
  EXPECT_EQ(selectValues("amount IN (1)"), (std::vector<std::string>{"1"}));
  EXPECT_EQ(selectValues("amount BETWEEN 0 AND 1"),
            (std::vector<std::string>{"0.1", "0.2", "0.3",
                                      "0.3000000000000000000001", "1"}));
}

TEST(ExecuteQueryActionNumericTest,
     UsesPostgresStyleSignificantDigitsForNumericDivision) {
  auto port = makeNumericPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto inserted =
      action.execute("INSERT INTO decimals (amount) VALUES (1), (1000000)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          inserted));
  port.rows = port.insertedRows;

  const auto selected = action.execute("SELECT amount / 3 FROM decimals");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          selected));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(selected).rows;
  ASSERT_EQ(rows.size(), 2U);
  EXPECT_EQ(formatSqlNumericValue(std::get<SqlNumericValue>(rows[0][0])),
            "0.33333333333333333333");
  EXPECT_EQ(formatSqlNumericValue(std::get<SqlNumericValue>(rows[1][0])),
            "333333.333333333333");
}
