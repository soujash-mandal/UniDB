#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, ReturnsStructuredParseAndExecutionErrors) {
  // Scenario 1: Parse and downstream failures are values, not uncaught errors.
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto syntax = action.execute("CREATE TABLE");
  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(
      syntax));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(syntax).code,
            QueryExecutionService::QueryExecutionErrorCode::SyntaxError);

  port.fail = true;
  const auto failure = action.execute("CREATE TABLE people (id INT);");
  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(
      failure));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(failure).code,
            QueryExecutionService::QueryExecutionErrorCode::ExecutionError);
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(failure).message,
            "fake RDB failure");
}
