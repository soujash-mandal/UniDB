#include "../../adapter/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, ReturnsColumnAliasesInResultMetadata) {
  FakeQesRdbServicePort port;
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{7}, std::string("Ada")}));
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT name AS username, id AS user_id FROM people;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 2U);
  EXPECT_EQ(result.columns[0].name, "username");
  EXPECT_EQ(result.columns[0].type, unidb::sql::SqlType::Text);
  EXPECT_EQ(result.columns[1].name, "user_id");
  EXPECT_EQ(result.columns[1].type, unidb::sql::SqlType::Int);
  ASSERT_EQ(result.rows.size(), 1U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "Ada");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]), 7);
}

TEST(ExecuteQueryActionTest, RejectsDuplicateOutputAliasesBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT name AS label, id AS label FROM people;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("Duplicate SELECT output column name: label"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}

TEST(ExecuteQueryActionTest, RejectsAliasCollidingWithAnotherOutputName) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute("SELECT name AS id, id FROM people;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("Duplicate SELECT output column name: id"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}
