#include "../../domain/codec/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

#include <vector>

namespace {
FakeQesRdbServicePort makeBooleanPort() {
  FakeQesRdbServicePort port;
  port.schema = {
      {"enabled", unidb::sql::SqlType::Int,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Boolean, {}}}};
  port.rows = {SqlTupleCodec::encode(port.schema, {false}),
               SqlTupleCodec::encode(port.schema, {true}),
               SqlTupleCodec::encode(port.schema, {false})};
  return port;
}
} // namespace

TEST(ExecuteQueryActionBooleanTest, ProjectsBooleanValuesWithTheirRuntimeType) {
  auto port = makeBooleanPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute("SELECT enabled FROM flags;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_FALSE(std::get<bool>(result.rows[0][0]));
  EXPECT_TRUE(std::get<bool>(result.rows[1][0]));
  EXPECT_FALSE(std::get<bool>(result.rows[2][0]));
}

TEST(ExecuteQueryActionBooleanTest, FiltersBooleanEqualityAndInequality) {
  auto port = makeBooleanPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto trueRows =
      action.execute("SELECT enabled FROM flags WHERE enabled = TRUE;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          trueRows));
  const auto &trueResult =
      std::get<QueryExecutionService::QueryExecutionResult>(trueRows);
  ASSERT_EQ(trueResult.rows.size(), 1U);
  EXPECT_TRUE(std::get<bool>(trueResult.rows[0][0]));

  const auto falseRows =
      action.execute("SELECT enabled FROM flags WHERE enabled != TRUE;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          falseRows));
  const auto &falseResult =
      std::get<QueryExecutionService::QueryExecutionResult>(falseRows);
  ASSERT_EQ(falseResult.rows.size(), 2U);
  EXPECT_FALSE(std::get<bool>(falseResult.rows[0][0]));
  EXPECT_FALSE(std::get<bool>(falseResult.rows[1][0]));
}

TEST(ExecuteQueryActionBooleanTest, SupportsBooleanInAndNotInPredicates) {
  auto port = makeBooleanPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto in = action.execute(
      "SELECT enabled FROM flags WHERE enabled IN (TRUE) ORDER BY enabled;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(in));
  const auto &inRows =
      std::get<QueryExecutionService::QueryExecutionResult>(in).rows;
  ASSERT_EQ(inRows.size(), 1U);
  EXPECT_TRUE(std::get<bool>(inRows[0][0]));

  const auto notIn =
      action.execute("SELECT enabled FROM flags WHERE enabled NOT IN (TRUE);");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          notIn));
  const auto &notInRows =
      std::get<QueryExecutionService::QueryExecutionResult>(notIn).rows;
  ASSERT_EQ(notInRows.size(), 2U);
  EXPECT_FALSE(std::get<bool>(notInRows[0][0]));
  EXPECT_FALSE(std::get<bool>(notInRows[1][0]));
}

TEST(ExecuteQueryActionBooleanTest, OrdersFalseBeforeTrue) {
  auto port = makeBooleanPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT enabled FROM flags ORDER BY enabled ASC;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome).rows;
  ASSERT_EQ(rows.size(), 3U);
  EXPECT_FALSE(std::get<bool>(rows[0][0]));
  EXPECT_FALSE(std::get<bool>(rows[1][0]));
  EXPECT_TRUE(std::get<bool>(rows[2][0]));
}

TEST(ExecuteQueryActionBooleanTest, SupportsDistinctAndGroupingByBoolean) {
  auto port = makeBooleanPort();
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto distinct =
      action.execute("SELECT DISTINCT enabled FROM flags ORDER BY enabled;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          distinct));
  const auto &distinctRows =
      std::get<QueryExecutionService::QueryExecutionResult>(distinct).rows;
  ASSERT_EQ(distinctRows.size(), 2U);
  EXPECT_FALSE(std::get<bool>(distinctRows[0][0]));
  EXPECT_TRUE(std::get<bool>(distinctRows[1][0]));

  const auto grouped = action.execute(
      "SELECT enabled, COUNT(*) FROM flags GROUP BY enabled ORDER BY enabled;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          grouped));
  const auto &groupedRows =
      std::get<QueryExecutionService::QueryExecutionResult>(grouped).rows;
  ASSERT_EQ(groupedRows.size(), 2U);
  EXPECT_FALSE(std::get<bool>(groupedRows[0][0]));
  EXPECT_EQ(std::get<std::int64_t>(groupedRows[0][1]), 2);
  EXPECT_TRUE(std::get<bool>(groupedRows[1][0]));
  EXPECT_EQ(std::get<std::int64_t>(groupedRows[1][1]), 1);
}


TEST(ExecuteQueryActionBooleanTest,
     RejectsUnionBetweenBooleanAndIntegerColumns) {
  FakeQesRdbServicePort port;
  port.schema = {
      {"id", unidb::sql::SqlType::Int,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Integer, {}}},
      {"enabled", unidb::sql::SqlType::Int,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Boolean, {}}}};
  port.rows = {
      SqlTupleCodec::encode(port.schema, {std::int64_t{1}, false}),
      SqlTupleCodec::encode(port.schema, {std::int64_t{2}, true})};

  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  const auto outcome = action.execute(
      "SELECT enabled FROM flags UNION SELECT id FROM flags;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("matching types"),
            std::string::npos);
}

TEST(ExecuteQueryActionBooleanTest,
     SupportsUnionAndUnionAllBetweenBooleanColumns) {
  FakeQesRdbServicePort port;
  port.schema = {
      {"enabled", unidb::sql::SqlType::Int,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Boolean, {}}}};
  port.rows = {SqlTupleCodec::encode(port.schema, {false}),
               SqlTupleCodec::encode(port.schema, {true})};

  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto unionOutcome = action.execute(
      "SELECT enabled FROM flags WHERE enabled = FALSE UNION "
      "SELECT enabled FROM flags WHERE enabled = TRUE;");
  ASSERT_TRUE(std::holds_alternative<
              QueryExecutionService::QueryExecutionResult>(unionOutcome));
  const auto &unionRows =
      std::get<QueryExecutionService::QueryExecutionResult>(unionOutcome).rows;
  ASSERT_EQ(unionRows.size(), 2U);
  EXPECT_FALSE(std::get<bool>(unionRows[0][0]));
  EXPECT_TRUE(std::get<bool>(unionRows[1][0]));

  const auto unionAllOutcome = action.execute(
      "SELECT enabled FROM flags WHERE enabled = FALSE UNION ALL "
      "SELECT enabled FROM flags WHERE enabled = FALSE;");
  ASSERT_TRUE(std::holds_alternative<
              QueryExecutionService::QueryExecutionResult>(unionAllOutcome));
  const auto &unionAllRows =
      std::get<QueryExecutionService::QueryExecutionResult>(unionAllOutcome)
          .rows;
  ASSERT_EQ(unionAllRows.size(), 2U);
  EXPECT_FALSE(std::get<bool>(unionAllRows[0][0]));
  EXPECT_FALSE(std::get<bool>(unionAllRows[1][0]));
}
