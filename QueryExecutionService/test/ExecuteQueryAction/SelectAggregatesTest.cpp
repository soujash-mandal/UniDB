#include "../../adapter/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, CountsAllRowsAndFilteredRows) {
  FakeQesRdbServicePort port;
  port.rows = {SqlTupleCodec::encode(
                   {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
                   {std::int64_t{3}, std::string("Ada")}),
               SqlTupleCodec::encode(
                   {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
                   {std::int64_t{7}, std::string("Bob")})};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute("SELECT COUNT(*) AS total FROM people;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 1U);
  EXPECT_EQ(result.columns[0].name, "total");
  EXPECT_EQ(result.columns[0].type, unidb::sql::SqlType::Int);
  ASSERT_EQ(result.rows.size(), 1U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 2);

  const auto filtered =
      action.execute("SELECT COUNT(id) FROM people WHERE id > 3;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          filtered));
  EXPECT_EQ(std::get<std::int64_t>(
                std::get<QueryExecutionService::QueryExecutionResult>(filtered)
                    .rows[0][0]),
            1);
}

TEST(ExecuteQueryActionTest, CalculatesMinMaxSumAndIntegerAverage) {
  FakeQesRdbServicePort port;
  port.rows = {SqlTupleCodec::encode(
                   {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
                   {std::int64_t{3}, std::string("Ada")}),
               SqlTupleCodec::encode(
                   {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
                   {std::int64_t{8}, std::string("Bob")}),
               SqlTupleCodec::encode(
                   {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
                   {std::int64_t{4}, std::string("Cara")})};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto min = action.execute("SELECT MIN(id) FROM people;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(min));
  EXPECT_EQ(std::get<std::int64_t>(
                std::get<QueryExecutionService::QueryExecutionResult>(min)
                    .rows[0][0]),
            3);

  const auto max = action.execute("SELECT MAX(name) FROM people;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(max));
  EXPECT_EQ(std::get<std::string>(
                std::get<QueryExecutionService::QueryExecutionResult>(max)
                    .rows[0][0]),
            "Cara");

  const auto sum = action.execute("SELECT SUM(id) FROM people;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(sum));
  EXPECT_EQ(std::get<std::int64_t>(
                std::get<QueryExecutionService::QueryExecutionResult>(sum)
                    .rows[0][0]),
            15);

  const auto avg = action.execute("SELECT AVG(id) FROM people;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(avg));
  EXPECT_EQ(std::get<std::int64_t>(
                std::get<QueryExecutionService::QueryExecutionResult>(avg)
                    .rows[0][0]),
            5);
}

TEST(ExecuteQueryActionTest,
     RejectsInvalidAggregateArgumentsBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  for (const auto *sql :
       {"SELECT SUM(name) FROM people;", "SELECT AVG(name) FROM people;",
        "SELECT MIN(missing) FROM people;"}) {
    const auto outcome = action.execute(sql);
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionError>(
            outcome))
        << sql;
    EXPECT_EQ(port.selectCalls, 0U) << sql;
  }
}

TEST(ExecuteQueryActionTest,
     CountReturnsZeroForEmptyInputAndOtherAggregatesFail) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto count = action.execute("SELECT COUNT(*) FROM people;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          count));
  EXPECT_EQ(std::get<std::int64_t>(
                std::get<QueryExecutionService::QueryExecutionResult>(count)
                    .rows[0][0]),
            0);

  for (const auto *sql :
       {"SELECT MIN(id) FROM people;", "SELECT MAX(id) FROM people;",
        "SELECT SUM(id) FROM people;", "SELECT AVG(id) FROM people;"}) {
    const auto outcome = action.execute(sql);
    EXPECT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionError>(
            outcome))
        << sql;
  }
}
