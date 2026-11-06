#include "../../adapter/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <variant>

TEST(ExecuteQueryActionTextFunctionsTest, EvaluatesUpperLowerAndByteLength) {
  FakeQesRdbServicePort port;
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{7}, std::string("MiXeD text")}));
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT UPPER(name), LOWER(name), LENGTH(name) FROM people");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 3U);
  EXPECT_EQ(result.columns[0].name, "UPPER(name)");
  EXPECT_EQ(result.columns[0].type, unidb::sql::SqlType::Text);
  EXPECT_EQ(result.columns[1].name, "LOWER(name)");
  EXPECT_EQ(result.columns[1].type, unidb::sql::SqlType::Text);
  EXPECT_EQ(result.columns[2].name, "LENGTH(name)");
  EXPECT_EQ(result.columns[2].type, unidb::sql::SqlType::Int);
  ASSERT_EQ(result.rows.size(), 1U);
  ASSERT_EQ(result.rows[0].size(), 3U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "MIXED TEXT");
  EXPECT_EQ(std::get<std::string>(result.rows[0][1]), "mixed text");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][2]), 10);
}

TEST(ExecuteQueryActionTextFunctionsTest, SupportsAliasesAndQualifiedColumns) {
  FakeQesRdbServicePort port;
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{7}, std::string("UniDB")}));
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT UPPER(p.name) AS label FROM people AS p");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 1U);
  EXPECT_EQ(result.columns[0].name, "label");
  ASSERT_EQ(result.rows.size(), 1U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "UNIDB");
}

TEST(ExecuteQueryActionTextFunctionsTest, RejectsIntegerColumnArguments) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute("SELECT UPPER(id) FROM people");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("require TEXT columns"),
            std::string::npos);
}
