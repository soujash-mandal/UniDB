#include "ExecuteQueryActionTestSupport.h"

#include "../../domain/codec/SqlTupleCodec.h"

#include <gtest/gtest.h>

#include <variant>

namespace {

unidb::sql::ColumnDefinition makeColumn(const std::string &name,
                                        unidb::sql::SqlType type,
                                        unidb::sql::SqlTypeId declaredType,
                                        bool nullable) {
  unidb::sql::ColumnDefinition column;
  column.name = name;
  column.type = type;
  column.declaredType = unidb::sql::SqlTypeDescriptor{declaredType, {}};
  column.nullable = nullable;
  return column;
}

TEST(ExecuteQueryActionNullTest, InsertsAndReadsBackNullForNullableColumn) {
  FakeQesRdbServicePort port;
  port.schema = {
      makeColumn("id", unidb::sql::SqlType::Int,
                 unidb::sql::SqlTypeId::Integer, false),
      makeColumn("name", unidb::sql::SqlType::Text,
                 unidb::sql::SqlTypeId::Text, true)};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "INSERT INTO users (id, name) VALUES (1, NULL), (2, 'Ada')");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  ASSERT_EQ(port.insertedRows.size(), 2U);
  const auto first = SqlTupleCodec::decode(port.schema, port.insertedRows[0]);
  const auto second = SqlTupleCodec::decode(port.schema, port.insertedRows[1]);
  ASSERT_EQ(first.size(), 2U);
  EXPECT_TRUE(std::holds_alternative<unidb::sql::SqlNullValue>(first[1]));
  EXPECT_EQ(std::get<std::string>(second[1]), "Ada");
}

TEST(ExecuteQueryActionNullTest, RejectsNullForNotNullColumnBeforeAnyInsert) {
  FakeQesRdbServicePort port;
  port.schema = {
      makeColumn("id", unidb::sql::SqlType::Int,
                 unidb::sql::SqlTypeId::Integer, false),
      makeColumn("name", unidb::sql::SqlType::Text,
                 unidb::sql::SqlTypeId::Text, false)};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "INSERT INTO users (id, name) VALUES (1, 'Ada'), (2, NULL)");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_EQ(port.insertCalls, 0U);
}

} // namespace
