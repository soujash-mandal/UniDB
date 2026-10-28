#include "ExecuteQueryActionTestSupport.h"
#include "../../adapter/SqlTupleCodec.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, SelectStarDecodesRows) {
  // Scenario 1: SELECT * returns schema names and decoded logical values.
  FakeQesRdbServicePort port;
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{9}, std::string("Lin")}));
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute("SELECT * FROM people;");

  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
      outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  EXPECT_EQ(result.kind, QueryExecutionService::QueryExecutionResultKind::Rows);
  ASSERT_EQ(result.columns.size(), 2U);
  EXPECT_EQ(result.columns[0].name, "id");
  ASSERT_EQ(result.rows.size(), 1U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 9);
  EXPECT_EQ(std::get<std::string>(result.rows[0][1]), "Lin");
}
