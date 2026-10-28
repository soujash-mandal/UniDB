#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, RejectsUnsupportedSelectForms) {
  // Scenario 1: V1 QES does not silently claim projection or WHERE support.
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto projection = action.execute("SELECT name FROM people;");
  const auto predicate = action.execute("SELECT * FROM people WHERE id = 1;");

  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(
      projection));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(projection).code,
            QueryExecutionService::QueryExecutionErrorCode::UnsupportedStatement);
  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(
      predicate));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(predicate).code,
            QueryExecutionService::QueryExecutionErrorCode::UnsupportedStatement);
  EXPECT_EQ(port.selectCalls, 0);
}
