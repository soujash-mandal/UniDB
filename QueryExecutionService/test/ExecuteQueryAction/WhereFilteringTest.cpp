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

TEST(ExecuteQueryActionTest, CombinesWherePredicatesWithAndAndOr) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, "A"}, {2, "B"}, {3, "B"}, {4, "C"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  const auto andOutcome =
      action.execute("SELECT id FROM people WHERE id >= 2 AND name = 'B';");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          andOutcome));
  const auto &andResult =
      std::get<QueryExecutionService::QueryExecutionResult>(andOutcome);
  ASSERT_EQ(andResult.rows.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(andResult.rows[0][0]), 2);
  EXPECT_EQ(std::get<std::int64_t>(andResult.rows[1][0]), 3);

  const auto orOutcome =
      action.execute("SELECT id FROM people WHERE id = 1 OR id = 4;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          orOutcome));
  const auto &orResult =
      std::get<QueryExecutionService::QueryExecutionResult>(orOutcome);
  ASSERT_EQ(orResult.rows.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(orResult.rows[0][0]), 1);
  EXPECT_EQ(std::get<std::int64_t>(orResult.rows[1][0]), 4);
}

TEST(ExecuteQueryActionTest, AppliesAndBeforeOrWhenEvaluatingWhere) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, "A"}, {2, "B"}, {3, "B"}, {4, "C"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  const auto outcome = action.execute(
      "SELECT id FROM people WHERE id = 1 OR id >= 2 AND name = 'B';");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 1);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][0]), 2);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[2][0]), 3);
}

TEST(ExecuteQueryActionTest, ValidatesEveryCompoundPredicateBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  const auto outcome =
      action.execute("SELECT * FROM people WHERE id = 1 OR missing = 2;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("Unknown WHERE column: missing"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}

TEST(ExecuteQueryActionTest, SortsRowsAscendingAndDescending) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {3, "C"}, {1, "A"}, {4, "D"}, {2, "B"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto ascending =
      action.execute("SELECT id FROM people ORDER BY id ASC;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          ascending));
  const auto &ascendingResult =
      std::get<QueryExecutionService::QueryExecutionResult>(ascending);
  ASSERT_EQ(ascendingResult.rows.size(), 4U);
  for (std::size_t i = 0; i < ascendingResult.rows.size(); ++i) {
    EXPECT_EQ(std::get<std::int64_t>(ascendingResult.rows[i][0]),
              static_cast<std::int64_t>(i + 1));
  }

  const auto descending =
      action.execute("SELECT id FROM people ORDER BY id DESC;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          descending));
  const auto &descendingResult =
      std::get<QueryExecutionService::QueryExecutionResult>(descending);
  ASSERT_EQ(descendingResult.rows.size(), 4U);
  for (std::size_t i = 0; i < descendingResult.rows.size(); ++i) {
    EXPECT_EQ(std::get<std::int64_t>(descendingResult.rows[i][0]),
              static_cast<std::int64_t>(4 - i));
  }
}

TEST(ExecuteQueryActionTest,
     SortsByMultipleColumnsAndAllowsUnselectedSortColumn) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {3, "B"}, {1, "A"}, {4, "B"}, {2, "A"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT id FROM people ORDER BY name ASC, id DESC;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 4U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 2);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][0]), 1);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[2][0]), 4);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[3][0]), 3);
  ASSERT_EQ(result.columns.size(), 1U);
  EXPECT_EQ(result.columns[0].name, "id");
}

TEST(ExecuteQueryActionTest, AppliesWhereFilteringBeforeOrdering) {
  FakeQesRdbServicePort port;
  for (const std::int64_t id : {4, 1, 3, 2}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
        {id, std::string("user") + std::to_string(id)}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT id FROM people WHERE id > 1 ORDER BY id ASC;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 2);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][0]), 3);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[2][0]), 4);
}

TEST(ExecuteQueryActionTest, RejectsUnknownOrderByColumnBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT id FROM people ORDER BY missing DESC;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("Unknown ORDER BY column: missing"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}

TEST(ExecuteQueryActionTest, AppliesLimitAndOffsetAfterFilteringAndOrdering) {
  FakeQesRdbServicePort port;
  for (const std::int64_t id : {5, 1, 4, 2, 3}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
        {id, std::string("user") + std::to_string(id)}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT id FROM people WHERE id > 1 ORDER BY id DESC LIMIT 2 OFFSET 1;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 4);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][0]), 3);
}

TEST(ExecuteQueryActionTest, HandlesZeroLimitAndOffsetBeyondEnd) {
  FakeQesRdbServicePort port;
  for (const std::int64_t id : {1, 2, 3}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
        {id, std::string("user") + std::to_string(id)}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto zeroLimit = action.execute("SELECT * FROM people LIMIT 0;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          zeroLimit));
  EXPECT_TRUE(std::get<QueryExecutionService::QueryExecutionResult>(zeroLimit)
                  .rows.empty());

  const auto beyondEnd = action.execute("SELECT * FROM people OFFSET 10;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          beyondEnd));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(beyondEnd);
  EXPECT_TRUE(result.rows.empty());
  ASSERT_EQ(result.columns.size(), 2U);
}

TEST(ExecuteQueryActionTest, AppliesOffsetWithoutLimitAndDefaultsOffsetToZero) {
  FakeQesRdbServicePort port;
  for (const std::int64_t id : {1, 2, 3}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
        {id, std::string("user") + std::to_string(id)}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto offset = action.execute("SELECT id FROM people OFFSET 1;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          offset));
  const auto &offsetResult =
      std::get<QueryExecutionService::QueryExecutionResult>(offset);
  ASSERT_EQ(offsetResult.rows.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(offsetResult.rows[0][0]), 2);

  const auto limit = action.execute("SELECT id FROM people LIMIT 1;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          limit));
  const auto &limitResult =
      std::get<QueryExecutionService::QueryExecutionResult>(limit);
  ASSERT_EQ(limitResult.rows.size(), 1U);
  EXPECT_EQ(std::get<std::int64_t>(limitResult.rows[0][0]), 1);
}

TEST(ExecuteQueryActionTest, FiltersIntegerAndTextValuesWithInAndNotIn) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, "Ada"}, {2, "Grace"}, {3, "Linus"}, {4, "Ken"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto integerIn = action.execute(
      "SELECT id FROM people WHERE id IN (1, 3, 99) ORDER BY id;");
  ASSERT_TRUE(std::holds_alternative<
              QueryExecutionService::QueryExecutionResult>(integerIn));
  const auto &integerInResult =
      std::get<QueryExecutionService::QueryExecutionResult>(integerIn);
  ASSERT_EQ(integerInResult.rows.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(integerInResult.rows[0][0]), 1);
  EXPECT_EQ(std::get<std::int64_t>(integerInResult.rows[1][0]), 3);

  const auto textNotIn = action.execute(
      "SELECT name FROM people WHERE name NOT IN ('Ada', 'Linus') ORDER BY id;");
  ASSERT_TRUE(std::holds_alternative<
              QueryExecutionService::QueryExecutionResult>(textNotIn));
  const auto &textNotInResult =
      std::get<QueryExecutionService::QueryExecutionResult>(textNotIn);
  ASSERT_EQ(textNotInResult.rows.size(), 2U);
  EXPECT_EQ(std::get<std::string>(textNotInResult.rows[0][0]), "Grace");
  EXPECT_EQ(std::get<std::string>(textNotInResult.rows[1][0]), "Ken");
}

TEST(ExecuteQueryActionTest, SupportsInPredicatesInsideNestedWhereExpressions) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, "Ada"}, {2, "Grace"}, {3, "Linus"}, {4, "Ken"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT id FROM people WHERE (id IN (1, 2) OR name NOT IN ('Ken')) "
      "AND id > 1 ORDER BY id;");
  ASSERT_TRUE(std::holds_alternative<
              QueryExecutionService::QueryExecutionResult>(outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 2);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][0]), 3);
}

TEST(ExecuteQueryActionTest, ValidatesAllInListValuesBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT * FROM people WHERE id IN (1, 'wrong', 3);");

  ASSERT_TRUE(std::holds_alternative<
              QueryExecutionService::QueryExecutionError>(outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("WHERE value type does not match column: id"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}

TEST(ExecuteQueryActionTest, RejectsUnknownColumnForInBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT * FROM people WHERE missing IN (1, 2);");

  ASSERT_TRUE(std::holds_alternative<
              QueryExecutionService::QueryExecutionError>(outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("Unknown WHERE column: missing"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}
