#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, CreatesTableAndReturnsCommandResult) {
  // Scenario 1: CREATE TABLE dispatches its typed schema to RDBService.
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute("CREATE TABLE people (id INT, name TEXT);");

  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
      outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  EXPECT_EQ(result.kind, QueryExecutionService::QueryExecutionResultKind::Command);
  EXPECT_EQ(port.createTableCalls, 1);
  EXPECT_EQ(port.tableName, "people");
  ASSERT_EQ(port.createdColumns.size(), 2U);
  EXPECT_EQ(port.createdColumns[0].type, unidb::sql::SqlType::Int);
  EXPECT_EQ(port.createdColumns[1].type, unidb::sql::SqlType::Text);
}
