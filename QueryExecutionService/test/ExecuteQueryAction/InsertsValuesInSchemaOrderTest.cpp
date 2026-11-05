#include "../../adapter/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, InsertsValuesInSchemaOrder) {
  // Scenario 1: INSERT columns are reordered to match the stored table schema.
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("INSERT INTO people (name, id) VALUES ('Ada', 7);");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto decoded = SqlTupleCodec::decode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      port.insertedBytes);
  ASSERT_EQ(decoded.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(decoded[0]), 7);
  EXPECT_EQ(std::get<std::string>(decoded[1]), "Ada");
  EXPECT_EQ(port.insertCalls, 1);
}

TEST(ExecuteQueryActionTest, InsertsMultipleRowsInSchemaOrder) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "INSERT INTO people (name, id) VALUES ('Ada', 7), ('Grace', 8);");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  EXPECT_EQ(result.affectedRows, 2U);
  EXPECT_EQ(result.message, "Rows inserted");
  ASSERT_EQ(port.insertedRows.size(), 2U);
  const auto first = SqlTupleCodec::decode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      port.insertedRows[0]);
  const auto second = SqlTupleCodec::decode(
      {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
      port.insertedRows[1]);
  EXPECT_EQ(std::get<std::int64_t>(first[0]), 7);
  EXPECT_EQ(std::get<std::string>(first[1]), "Ada");
  EXPECT_EQ(std::get<std::int64_t>(second[0]), 8);
  EXPECT_EQ(std::get<std::string>(second[1]), "Grace");
  EXPECT_EQ(port.insertCalls, 2U);
}

TEST(ExecuteQueryActionTest, ValidatesEveryInsertRowBeforeWritingAnyRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "INSERT INTO people (id, name) VALUES (1, 'Ada'), ('wrong', 'Grace');");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_EQ(port.insertCalls, 0U);
}

TEST(ExecuteQueryActionTest, ReportsPartialCommitWhenLaterInsertFails) {
  FakeQesRdbServicePort port;
  port.failOnInsertCall = 2;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "INSERT INTO people (id, name) VALUES (1, 'Ada'), (2, 'Grace');");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  const auto &error =
      std::get<QueryExecutionService::QueryExecutionError>(outcome);
  EXPECT_NE(error.message.find("failed after 1 row(s) were inserted"),
            std::string::npos);
  EXPECT_EQ(port.insertCalls, 2U);
}

TEST(ExecuteQueryActionTest, CountsRowCommittedBeforePostInsertFailure) {
  FakeQesRdbServicePort port;
  port.commitThenFailOnInsertCall = 2;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "INSERT INTO people (id, name) VALUES (1, 'Ada'), (2, 'Grace');");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  const auto &error =
      std::get<QueryExecutionService::QueryExecutionError>(outcome);
  EXPECT_NE(error.message.find("failed after 2 row(s) were inserted"),
            std::string::npos);
}
