#include "../../domain/codec/SqlTupleCodec.h"
#include "../../domain/planning/SqlStatementRules.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

#include <string>
#include <variant>

TEST(ExecuteQueryActionCharacterTest, PadsCharAndPreservesVarcharAndText) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto created =
      action.execute("CREATE TABLE labels (fixed CHAR(5), bounded VARCHAR(8), "
                     "unbounded VARCHAR, description TEXT)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          created));
  ASSERT_EQ(port.createdColumns.size(), 4U);
  port.schema = port.createdColumns;

  const auto inserted = action.execute(
      "INSERT INTO labels (fixed, bounded, unbounded, description) "
      "VALUES ('cat', 'éclair', '  keep  ', 'hello 🌍')");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          inserted))
      << std::get<QueryExecutionService::QueryExecutionError>(inserted).message;
  ASSERT_EQ(port.insertedRows.size(), 1U);

  const auto row = SqlTupleCodec::decode(port.schema, port.insertedRows[0]);
  ASSERT_EQ(row.size(), 4U);
  EXPECT_EQ(std::get<std::string>(row[0]), "cat  ");
  EXPECT_EQ(std::get<std::string>(row[1]), "éclair");
  EXPECT_EQ(std::get<std::string>(row[2]), "  keep  ");
  EXPECT_EQ(std::get<std::string>(row[3]), "hello 🌍");
}

TEST(ExecuteQueryActionCharacterTest, RejectsVarcharOverflowByCharacters) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto created =
      action.execute("CREATE TABLE labels (bounded VARCHAR(1))");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          created));
  port.schema = port.createdColumns;

  const auto overflow =
      action.execute("INSERT INTO labels (bounded) VALUES ('é猫')");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          overflow));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(overflow)
                .message.find("exceeds declared length"),
            std::string::npos);
  EXPECT_EQ(port.insertCalls, 0U);
}

TEST(ExecuteQueryActionCharacterTest, RejectsMalformedUtf8BeforeWriting) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlTypeModifier modifier;
  modifier.length = 2;
  port.schema = {{"value", unidb::sql::SqlType::Text,
                  unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Varchar,
                                                modifier}}};
  // SQL literals are valid UTF-8 by construction; exercise the domain
  // assignment boundary directly for malformed byte sequences instead.
  unidb::sql::InsertStatement statement;
  statement.tableName = "labels";
  statement.columnNames = {"value"};
  statement.rows = {{std::string("\xc0", 1)}};
  EXPECT_THROW(
      QueryExecutionService::domain::prepareInsertRows(statement, port.schema),
      std::invalid_argument);
}
