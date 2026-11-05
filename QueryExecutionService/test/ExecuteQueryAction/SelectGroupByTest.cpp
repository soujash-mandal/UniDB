#include "../../adapter/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, GroupsRowsAndAppliesHavingAggregateFilter) {
  FakeQesRdbServicePort port;
  port.schema = {{"department", unidb::sql::SqlType::Text},
                 {"salary", unidb::sql::SqlType::Int}};
  const auto types = std::vector<unidb::sql::SqlType>{unidb::sql::SqlType::Text,
                                                      unidb::sql::SqlType::Int};
  port.rows = {
      SqlTupleCodec::encode(types, {std::string("eng"), std::int64_t{10}}),
      SqlTupleCodec::encode(types, {std::string("eng"), std::int64_t{20}}),
      SqlTupleCodec::encode(types, {std::string("sales"), std::int64_t{5}}),
      SqlTupleCodec::encode(types, {std::string("sales"), std::int64_t{7}}),
      SqlTupleCodec::encode(types, {std::string("support"), std::int64_t{3}})};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT department, SUM(salary) AS payroll FROM employees "
                     "GROUP BY department HAVING SUM(salary) >= 12;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 2U);
  EXPECT_EQ(result.columns[0].name, "department");
  EXPECT_EQ(result.columns[1].name, "payroll");
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "eng");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]), 30);
  EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "sales");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][1]), 12);
}

TEST(ExecuteQueryActionTest, GroupsByMultipleColumnsAndSupportsCount) {
  FakeQesRdbServicePort port;
  port.schema = {{"department", unidb::sql::SqlType::Text},
                 {"level", unidb::sql::SqlType::Int},
                 {"salary", unidb::sql::SqlType::Int}};
  const auto types = std::vector<unidb::sql::SqlType>{unidb::sql::SqlType::Text,
                                                      unidb::sql::SqlType::Int,
                                                      unidb::sql::SqlType::Int};
  port.rows = {
      SqlTupleCodec::encode(
          types, {std::string("eng"), std::int64_t{1}, std::int64_t{10}}),
      SqlTupleCodec::encode(
          types, {std::string("eng"), std::int64_t{1}, std::int64_t{20}}),
      SqlTupleCodec::encode(
          types, {std::string("eng"), std::int64_t{2}, std::int64_t{5}})};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT department, level, COUNT(*) FROM employees "
                     "GROUP BY department, level;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][2]), 2);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][2]), 1);
}

TEST(ExecuteQueryActionTest, RejectsInvalidGroupProjectionBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  const auto outcome =
      action.execute("SELECT name, COUNT(*) FROM people GROUP BY id;");
  EXPECT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_EQ(port.selectCalls, 0U);
}
