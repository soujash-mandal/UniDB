#include "../../adapter/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, DistinctRemovesDuplicatesFromSelectedProjection) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, "Ada"}, {2, "Grace"}, {3, "Ada"}, {4, "Linus"},
           {5, "Grace"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT DISTINCT name FROM people ORDER BY name;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 1U);
  EXPECT_EQ(result.columns[0].name, "name");
  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "Ada");
  EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "Grace");
  EXPECT_EQ(std::get<std::string>(result.rows[2][0]), "Linus");
  EXPECT_EQ(port.selectCalls, 1U);
}

TEST(ExecuteQueryActionTest, DistinctUsesAllProjectedColumnsAndKeepsEmptyResults) {
  FakeQesRdbServicePort port;
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{1}, std::string("Ada")}));
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{2}, std::string("Ada")}));
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto distinctPair =
      action.execute("SELECT DISTINCT name, id FROM people ORDER BY id;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          distinctPair));
  const auto &pairRows =
      std::get<QueryExecutionService::QueryExecutionResult>(distinctPair).rows;
  ASSERT_EQ(pairRows.size(), 2U);

  const auto empty = action.execute(
      "SELECT DISTINCT name FROM people WHERE id > 100;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          empty));
  const auto &emptyResult =
      std::get<QueryExecutionService::QueryExecutionResult>(empty);
  ASSERT_EQ(emptyResult.columns.size(), 1U);
  EXPECT_TRUE(emptyResult.rows.empty());
}

TEST(ExecuteQueryActionTest, AppliesDistinctBeforeLimitAndOffset) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, "Ada"}, {2, "Ada"}, {3, "Grace"}, {4, "Linus"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT DISTINCT name FROM people ORDER BY name LIMIT 1 OFFSET 1;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome).rows;
  ASSERT_EQ(rows.size(), 1U);
  EXPECT_EQ(std::get<std::string>(rows[0][0]), "Grace");
}

TEST(ExecuteQueryActionTest, RejectsDistinctOrderByColumnOutsideProjection) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT DISTINCT name FROM people ORDER BY id;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find(
                    "ORDER BY column must appear in SELECT list when DISTINCT"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}
