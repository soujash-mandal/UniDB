#include "../../domain/codec/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <variant>

TEST(ExecuteQueryActionCaseWhenTest, EvaluatesFirstMatchingWhenAndElse) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, "Ada"}, {10, "Grace"}, {11, "Linus"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT id, CASE WHEN id > 10 THEN 'high' WHEN id = 10 THEN 'ten' "
      "ELSE 'low' END AS level FROM people ORDER BY id;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 2U);
  EXPECT_EQ(result.columns[0].name, "id");
  EXPECT_EQ(result.columns[1].name, "level");
  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][1]), "low");
  EXPECT_EQ(std::get<std::string>(result.rows[1][1]), "ten");
  EXPECT_EQ(std::get<std::string>(result.rows[2][1]), "high");
}

TEST(ExecuteQueryActionCaseWhenTest, SupportsIntegerCaseResults) {
  FakeQesRdbServicePort port;
  for (const std::int64_t id : {1, 2, 3}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
        {id, std::string("user")}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT CASE WHEN id >= 2 THEN 1 ELSE 0 END AS active FROM people;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome).rows;
  ASSERT_EQ(rows.size(), 3U);
  EXPECT_EQ(std::get<std::int64_t>(rows[0][0]), 0);
  EXPECT_EQ(std::get<std::int64_t>(rows[1][0]), 1);
  EXPECT_EQ(std::get<std::int64_t>(rows[2][0]), 1);
}

TEST(ExecuteQueryActionCaseWhenTest, FiltersRowsBeforeEvaluatingCase) {
  FakeQesRdbServicePort port;
  for (const std::int64_t id : {1, 2, 3}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
        {id, std::string("user")}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT CASE WHEN id > 1 THEN 'yes' ELSE 'no' END AS included "
      "FROM people WHERE id >= 2;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "yes");
  EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "yes");
}

TEST(ExecuteQueryActionCaseWhenTest,
     RejectsUnknownCaseColumnBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT CASE WHEN missing = 1 THEN 1 ELSE 0 END FROM people;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("Unknown WHERE column: missing"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}
