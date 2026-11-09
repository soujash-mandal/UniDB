#include "../../domain/codec/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionBooleanTest, CreatesBooleanColumnsUsingCanonicalType) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("CREATE TABLE flags (enabled BOOLEAN, visible BOOL)");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  ASSERT_EQ(port.createdColumns.size(), 2U);
  EXPECT_EQ(port.createdColumns[0].declaredType->id,
            unidb::sql::SqlTypeId::Boolean);
  EXPECT_EQ(port.createdColumns[1].declaredType->id,
            unidb::sql::SqlTypeId::Boolean);
}

TEST(ExecuteQueryActionBooleanTest, InsertsAndEncodesTrueAndFalseValues) {
  FakeQesRdbServicePort port;
  port.schema = {
      {"enabled", unidb::sql::SqlType::Int,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Boolean, {}}}};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("INSERT INTO flags (enabled) VALUES (TRUE), (false)");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionResult>(outcome)
                .affectedRows,
            2U);
  ASSERT_EQ(port.insertedRows.size(), 2U);
  const auto first = SqlTupleCodec::decode(port.schema, port.insertedRows[0]);
  const auto second = SqlTupleCodec::decode(port.schema, port.insertedRows[1]);
  ASSERT_EQ(first.size(), 1U);
  ASSERT_EQ(second.size(), 1U);
  EXPECT_TRUE(std::get<bool>(first[0]));
  EXPECT_FALSE(std::get<bool>(second[0]));
}

TEST(ExecuteQueryActionBooleanTest, RejectsIntegerAndTextForBooleanColumns) {
  for (const auto *literal : {"1", "'true'"}) {
    FakeQesRdbServicePort port;
    port.schema = {
        {"enabled", unidb::sql::SqlType::Int,
         unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Boolean, {}}}};
    unidb::sql::SqlParserImpl parser;
    auto action = makeQesAction(port, parser);

    const auto outcome = action.execute(
        std::string("INSERT INTO flags (enabled) VALUES (") + literal + ")");

    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionError>(
            outcome));
    EXPECT_EQ(port.insertCalls, 0U);
  }
}
