#include "../../domain/codec/SqlTupleCodec.h"
#include "ExecuteQueryActionTestSupport.h"

#include <gtest/gtest.h>

TEST(ExecuteQueryActionTest, SortsByIntegerProjectionAlias) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {2, "B"}, {1, "C"}, {3, "A"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT id + 10 AS score FROM people ORDER BY score DESC;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 13);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][0]), 12);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[2][0]), 11);
}

TEST(ExecuteQueryActionTest, SortsByTextFunctionProjectionAlias) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, "bravo"}, {2, "charlie"}, {3, "alpha"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT UPPER(name) AS label, id FROM people "
                     "ORDER BY label ASC;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "ALPHA");
  EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "BRAVO");
  EXPECT_EQ(std::get<std::string>(result.rows[2][0]), "CHARLIE");
}

TEST(ExecuteQueryActionTest, SortsByCaseProjectionAliasAndSecondaryColumn) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {2, "B"}, {1, "C"}, {3, "A"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT CASE WHEN id > 1 THEN 'later' ELSE 'first' END AS phase, id "
      "FROM people ORDER BY phase ASC, id DESC;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "first");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]), 1);
  EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "later");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][1]), 3);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[2][1]), 2);
}

TEST(ExecuteQueryActionTest, ProjectionAliasTakesPrecedenceOverSourceColumn) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, "C"}, {2, "A"}, {3, "B"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT name AS id, id AS original_id FROM people "
                     "ORDER BY id ASC;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "A");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]), 2);
  EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "B");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][1]), 3);
  EXPECT_EQ(std::get<std::string>(result.rows[2][0]), "C");
  EXPECT_EQ(std::get<std::int64_t>(result.rows[2][1]), 1);
}

TEST(ExecuteQueryActionTest, AppliesLimitAfterSortingByProjectionAlias) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, "C"}, {2, "A"}, {3, "B"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT name AS label FROM people ORDER BY label DESC LIMIT 2 OFFSET 1;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "B");
  EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "A");
}


TEST(ExecuteQueryActionTest, OrdersCharValuesUsingBlankPaddedSemantics) {
  FakeQesRdbServicePort port;
  port.schema = {
      {"name", unidb::sql::SqlType::Text,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Char, {4}}},
      {"id", unidb::sql::SqlType::Int}};
  const auto types =
      std::vector<unidb::sql::SqlType>{unidb::sql::SqlType::Text,
                                       unidb::sql::SqlType::Int};
  port.rows = {
      SqlTupleCodec::encode(types, {std::string("a  "), std::int64_t{1}}),
      SqlTupleCodec::encode(types, {std::string("a"), std::int64_t{2}}),
      SqlTupleCodec::encode(types, {std::string("b "), std::int64_t{3}})};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT name, id FROM people ORDER BY name ASC;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]), 1);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][1]), 2);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[2][1]), 3);
}

TEST(ExecuteQueryActionTest, OrdersVarcharValuesWithTrailingSpacesSignificant) {
  FakeQesRdbServicePort port;
  port.schema = {
      {"name", unidb::sql::SqlType::Text,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Varchar, {8}}},
      {"id", unidb::sql::SqlType::Int}};
  const auto types =
      std::vector<unidb::sql::SqlType>{unidb::sql::SqlType::Text,
                                       unidb::sql::SqlType::Int};
  port.rows = {
      SqlTupleCodec::encode(types, {std::string("a "), std::int64_t{1}}),
      SqlTupleCodec::encode(types, {std::string("a"), std::int64_t{2}})};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT name, id FROM people ORDER BY name DESC;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]), 1);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][1]), 2);
}

TEST(ExecuteQueryActionTest, AppliesCharacterSemanticsWhenOrderingByAlias) {
  FakeQesRdbServicePort port;
  port.schema = {
      {"name", unidb::sql::SqlType::Text,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Char, {4}}},
      {"id", unidb::sql::SqlType::Int}};
  const auto types =
      std::vector<unidb::sql::SqlType>{unidb::sql::SqlType::Text,
                                       unidb::sql::SqlType::Int};
  port.rows = {
      SqlTupleCodec::encode(types, {std::string("a  "), std::int64_t{1}}),
      SqlTupleCodec::encode(types, {std::string("a"), std::int64_t{2}})};
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT name AS label, id FROM people ORDER BY label;");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]), 1);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][1]), 2);
}
