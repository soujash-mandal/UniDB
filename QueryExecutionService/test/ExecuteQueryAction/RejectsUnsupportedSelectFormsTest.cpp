#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, RejectsWhereUntilFilteringIsImplemented) {
  // WHERE belongs to the next stacked PR; projection is supported here.
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto predicate = action.execute("SELECT * FROM people WHERE id = 1;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          predicate));
  EXPECT_EQ(
      std::get<QueryExecutionService::QueryExecutionError>(predicate).code,
      QueryExecutionService::QueryExecutionErrorCode::UnsupportedStatement);
  EXPECT_EQ(port.selectCalls, 0U);
}
