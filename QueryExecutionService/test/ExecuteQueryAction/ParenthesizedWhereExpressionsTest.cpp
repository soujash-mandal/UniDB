#include "../../adapter/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

TEST(ExecuteQueryActionTest, ExecutesParenthesizedWhereExpressions) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, "A"}, {2, "B"}, {3, "B"}, {4, "C"}, {5, "A"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT id FROM people WHERE ((id = 1 OR id = 2) AND name = 'A') "
      "OR (id >= 3 AND name = 'B') ORDER BY id;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 1);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][0]), 3);
}

TEST(ExecuteQueryActionTest, ParenthesesOverrideDefaultAndBeforeOrPrecedence) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, "A"}, {2, "B"}, {3, "B"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT id FROM people WHERE (id = 1 OR id = 2) AND name = 'B';");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 1U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 2);
}

TEST(ExecuteQueryActionTest, ValidatesNestedWherePredicatesBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT * FROM people WHERE (id = 1 OR (missing = 2 AND name = 'A'));");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("Unknown WHERE column: missing"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}

TEST(ExecuteQueryActionTest,
     RejectsMalformedParenthesizedWhereBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT * FROM people WHERE (id = 1 OR);");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(outcome).code,
            QueryExecutionService::QueryExecutionErrorCode::SyntaxError);
  EXPECT_EQ(port.selectCalls, 0U);
}
