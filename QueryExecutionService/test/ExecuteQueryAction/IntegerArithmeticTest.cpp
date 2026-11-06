#include "../../adapter/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <string>
#include <variant>

TEST(ExecuteQueryActionIntegerArithmeticTest,
     EvaluatesPrecedenceAndParentheses) {
  FakeQesRdbServicePort port;
  for (const std::int64_t id : {4, 5}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
        {id, std::string("user")}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute("SELECT id + 2 * 3 AS score, (id - 1) / "
                                      "2 AS half FROM people ORDER BY id");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 2U);
  EXPECT_EQ(result.columns[0].name, "score");
  EXPECT_EQ(result.columns[1].name, "half");
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 10);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]), 1);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][0]), 11);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][1]), 2);
}

TEST(ExecuteQueryActionIntegerArithmeticTest, ReportsDivisionByZero) {
  FakeQesRdbServicePort port;
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{7}, std::string("user")}));
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute("SELECT id / (id - 7) FROM people");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("Division by zero"),
            std::string::npos);
}

TEST(ExecuteQueryActionIntegerArithmeticTest, DetectsIntegerOverflow) {
  FakeQesRdbServicePort port;
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{1}, std::string("user")}));
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT id + 9223372036854775807 FROM people");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("INT64 range"),
            std::string::npos);
}

TEST(ExecuteQueryActionIntegerArithmeticTest,
     RejectsTextColumnArithmeticBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute("SELECT name + 1 FROM people");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_EQ(port.selectCalls, 0U);
}
