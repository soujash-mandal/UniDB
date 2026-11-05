#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, RejectsInnerJoinProjectionUntilNextStage) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto result =
      action.execute("SELECT users.id FROM users INNER JOIN orders "
                     "ON users.id = orders.id");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          result));
  const auto &error =
      std::get<QueryExecutionService::QueryExecutionError>(result);
  EXPECT_EQ(
      error.code,
      QueryExecutionService::QueryExecutionErrorCode::UnsupportedStatement);
  EXPECT_EQ(error.message, "INNER JOIN currently supports SELECT * only; column projection will be added in the next stage");
  EXPECT_EQ(port.getSchemaCalls, 2U);
  EXPECT_EQ(port.selectCalls, 0U);
}
