#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, RejectsParenthesizedWhereExpressions) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT * FROM people WHERE (id = 1 OR name = 'Lin');");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_EQ(
      std::get<QueryExecutionService::QueryExecutionError>(outcome).code,
      QueryExecutionService::QueryExecutionErrorCode::SyntaxError);
  EXPECT_EQ(port.selectCalls, 0U);
}
