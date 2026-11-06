#include "../../adapter/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

TEST(ExecuteQueryActionUnionTest, RemovesDuplicateRowsAndPreservesFirstOccurrence) {
  FakeQesRdbServicePort port;
  const std::vector<unidb::sql::SqlType> types{unidb::sql::SqlType::Int,
                                               unidb::sql::SqlType::Text};
  port.rows.push_back(
      SqlTupleCodec::encode(types, {std::int64_t{7}, std::string("Ada")}));
  port.rows.push_back(
      SqlTupleCodec::encode(types, {std::int64_t{8}, std::string("Grace")}));

  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  const auto outcome =
      action.execute("SELECT name FROM people WHERE id = 7 "
                     "UNION SELECT name FROM people WHERE id = 8 "
                     "UNION SELECT name FROM people WHERE id = 7;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 1U);
  EXPECT_EQ(result.columns[0].name, "name");
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "Ada");
  EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "Grace");
  EXPECT_EQ(port.selectCalls, 3U);
}

TEST(ExecuteQueryActionUnionTest, RejectsDifferentColumnCounts) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT id FROM people UNION SELECT id, name FROM people;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("same number of columns"),
            std::string::npos);
}

TEST(ExecuteQueryActionUnionTest, RejectsDifferentColumnTypes) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT id FROM people UNION SELECT name FROM people;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("matching types"),
            std::string::npos);
}
