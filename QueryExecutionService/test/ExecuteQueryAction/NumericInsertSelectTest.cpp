#include "../../domain/codec/SqlTupleCodec.h"
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
