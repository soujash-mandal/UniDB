#include "../../adapter/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, SelectsRequestedColumnsInRequestedOrder) {
  FakeQesRdbServicePort port;
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{9}, std::string("Lin")}));
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{10}, std::string("Mina")}));
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute("SELECT name, id FROM people;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  EXPECT_EQ(result.kind, QueryExecutionService::QueryExecutionResultKind::Rows);
  ASSERT_EQ(result.columns.size(), 2U);
  EXPECT_EQ(result.columns[0].name, "name");
  EXPECT_EQ(result.columns[1].name, "id");
  ASSERT_EQ(result.rows.size(), 2U);
  ASSERT_EQ(result.rows[0].size(), 2U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "Lin");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]), 9);
  EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "Mina");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][1]), 10);
  EXPECT_EQ(port.selectCalls, 1U);
}

TEST(ExecuteQueryActionTest, RejectsUnknownProjectedColumnBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute("SELECT missing FROM people;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_EQ(
      std::get<QueryExecutionService::QueryExecutionError>(outcome).code,
      QueryExecutionService::QueryExecutionErrorCode::ExecutionError);
  EXPECT_NE(
      std::get<QueryExecutionService::QueryExecutionError>(outcome).message
          .find("Unknown SELECT column: missing"),
      std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}

TEST(ExecuteQueryActionTest, PreservesProjectedColumnsWhenNoRowsMatch) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute("SELECT name FROM people;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 1U);
  EXPECT_EQ(result.columns[0].name, "name");
  EXPECT_TRUE(result.rows.empty());
}
