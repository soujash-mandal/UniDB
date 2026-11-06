#include "../../adapter/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionGroupedOrderLimitTest,
     OrdersGroupsByAggregateAliasDescending) {
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
                     "GROUP BY department ORDER BY payroll DESC;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "eng");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]), 30);
  EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "sales");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][1]), 12);
  EXPECT_EQ(std::get<std::string>(result.rows[2][0]), "support");
}

TEST(ExecuteQueryActionGroupedOrderLimitTest,
     SupportsQualifiedGroupColumnAndSecondaryAggregateOrdering) {
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
          types, {std::string("eng"), std::int64_t{2}, std::int64_t{30}}),
      SqlTupleCodec::encode(
          types, {std::string("sales"), std::int64_t{1}, std::int64_t{40}})};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT department, level, COUNT(*) AS total "
                     "FROM employees GROUP BY department, level "
                     "ORDER BY employees.department ASC, total DESC;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "eng");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]), 2);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][2]), 1);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][1]), 1);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][2]), 2);
  EXPECT_EQ(std::get<std::string>(result.rows[2][0]), "sales");
}

TEST(ExecuteQueryActionGroupedOrderLimitTest,
     AppliesOffsetAndLimitAfterGroupedOrdering) {
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
                     "GROUP BY department ORDER BY payroll DESC "
                     "LIMIT 1 OFFSET 1;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 1U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "sales");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]), 12);
}

TEST(ExecuteQueryActionGroupedOrderLimitTest,
     OffsetBeyondGroupCountReturnsNoRows) {
  FakeQesRdbServicePort port;
  port.schema = {{"department", unidb::sql::SqlType::Text},
                 {"salary", unidb::sql::SqlType::Int}};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT department, COUNT(*) FROM employees "
                     "GROUP BY department OFFSET 1;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  EXPECT_TRUE(std::get<QueryExecutionService::QueryExecutionResult>(outcome)
                  .rows.empty());
}
