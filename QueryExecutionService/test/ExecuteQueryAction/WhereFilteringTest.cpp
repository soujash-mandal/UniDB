#include "../../adapter/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, FiltersRowsWithAllComparisonOperators) {
  FakeQesRdbServicePort port;
  for (const std::int64_t id : {1, 2, 3}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
        {id, std::string("user") + std::to_string(id)}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const std::vector<std::pair<std::string, std::vector<std::int64_t>>> cases{
      {"=", {2}}, {"!=", {1, 3}}, {">", {3}},
      {"<", {1}}, {">=", {2, 3}}, {"<=", {1, 2}}};
  for (const auto &[op, expected] : cases) {
    const auto outcome =
        action.execute("SELECT id FROM people WHERE id " + op + " 2;");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            outcome))
        << "operator: " << op;
    const auto &result =
        std::get<QueryExecutionService::QueryExecutionResult>(outcome);
    std::vector<std::int64_t> actual;
    for (const auto &row : result.rows) {
      actual.push_back(std::get<std::int64_t>(row[0]));
    }
    EXPECT_EQ(actual, expected) << "operator: " << op;
  }
  EXPECT_EQ(port.selectCalls, 6U);
}

TEST(ExecuteQueryActionTest, FiltersTextColumnsLexicographically) {
  FakeQesRdbServicePort port;
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{1}, std::string("Amy")}));
  port.rows.push_back(SqlTupleCodec::encode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      {std::int64_t{2}, std::string("Zoe")}));
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT id FROM people WHERE name >= 'M';");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 1U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 2);
}

TEST(ExecuteQueryActionTest, RejectsUnknownWhereColumnBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT * FROM people WHERE missing = 1;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("Unknown WHERE column: missing"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}

TEST(ExecuteQueryActionTest, RejectsWhereValueWithWrongColumnType) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute("SELECT * FROM people WHERE name = 123;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("WHERE value type does not match column: name"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}

TEST(ExecuteQueryActionTest, ReturnsProjectedColumnsWhenWhereMatchesNoRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute("SELECT name FROM people WHERE id = 99;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 1U);
  EXPECT_EQ(result.columns[0].name, "name");
  EXPECT_TRUE(result.rows.empty());
}
