#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, ExecutesInnerJoinProjectionAfterProjectionStage) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto result =
      action.execute("SELECT users.id FROM users INNER JOIN orders "
                     "ON users.id = orders.id");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          result));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(result);
  EXPECT_EQ(rows.kind,
            QueryExecutionService::QueryExecutionResultKind::Rows);
  ASSERT_EQ(rows.columns.size(), 1U);
  EXPECT_EQ(rows.columns[0].name, "users.id");
  EXPECT_TRUE(rows.rows.empty());
  EXPECT_EQ(port.getSchemaCalls, 2U);
  EXPECT_EQ(port.selectCalls, 2U);
}
