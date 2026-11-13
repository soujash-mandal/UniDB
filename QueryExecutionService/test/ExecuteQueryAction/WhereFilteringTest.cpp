#include "../../domain/codec/SqlTupleCodec.h"
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

TEST(ExecuteQueryActionTest, FiltersInclusiveBetweenAndNotBetweenRanges) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, "Ada"}, {2, "Grace"}, {3, "Linus"}, {4, "Zoe"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto between = action.execute(
      "SELECT id FROM people WHERE id BETWEEN 2 AND 3 ORDER BY id;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          between));
  const auto &betweenResult =
      std::get<QueryExecutionService::QueryExecutionResult>(between);
  ASSERT_EQ(betweenResult.rows.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(betweenResult.rows[0][0]), 2);
  EXPECT_EQ(std::get<std::int64_t>(betweenResult.rows[1][0]), 3);

  const auto notBetween =
      action.execute("SELECT name FROM people WHERE name NOT BETWEEN 'Grace' "
                     "AND 'Linus' ORDER BY id;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          notBetween));
  const auto &notBetweenResult =
      std::get<QueryExecutionService::QueryExecutionResult>(notBetween);
  ASSERT_EQ(notBetweenResult.rows.size(), 2U);
  EXPECT_EQ(std::get<std::string>(notBetweenResult.rows[0][0]), "Ada");
  EXPECT_EQ(std::get<std::string>(notBetweenResult.rows[1][0]), "Zoe");
}

TEST(ExecuteQueryActionTest, HandlesEqualAndReversedBetweenBounds) {
  FakeQesRdbServicePort port;
  for (const std::int64_t id : {1, 2, 3}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
        {id, std::string("user") + std::to_string(id)}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto equal =
      action.execute("SELECT id FROM people WHERE id BETWEEN 2 AND 2;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          equal));
  const auto &equalResult =
      std::get<QueryExecutionService::QueryExecutionResult>(equal);
  ASSERT_EQ(equalResult.rows.size(), 1U);
  EXPECT_EQ(std::get<std::int64_t>(equalResult.rows[0][0]), 2);

  const auto reversed =
      action.execute("SELECT id FROM people WHERE id BETWEEN 3 AND 1;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          reversed));
  EXPECT_TRUE(std::get<QueryExecutionService::QueryExecutionResult>(reversed)
                  .rows.empty());
}

TEST(ExecuteQueryActionTest, SupportsBetweenInsideNestedWhereExpressions) {
  FakeQesRdbServicePort port;
  for (const std::int64_t id : {1, 2, 3, 4, 5}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text},
        {id, std::string("user") + std::to_string(id)}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  const auto outcome =
      action.execute("SELECT id FROM people WHERE (id BETWEEN 2 AND 4 OR id = "
                     "1) AND id NOT BETWEEN 3 AND 3 ORDER BY id;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 3U);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 1);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[1][0]), 2);
  EXPECT_EQ(std::get<std::int64_t>(result.rows[2][0]), 4);
}

TEST(ExecuteQueryActionTest, ValidatesBothBetweenBoundsBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  const auto outcome =
      action.execute("SELECT * FROM people WHERE id BETWEEN 1 AND 'wrong';");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("WHERE value type does not match column: id"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
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
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          integerIn));
  const auto &integerInResult =
      std::get<QueryExecutionService::QueryExecutionResult>(integerIn);
  ASSERT_EQ(integerInResult.rows.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(integerInResult.rows[0][0]), 1);
  EXPECT_EQ(std::get<std::int64_t>(integerInResult.rows[1][0]), 3);

  const auto textNotIn = action.execute("SELECT name FROM people WHERE name "
                                        "NOT IN ('Ada', 'Linus') ORDER BY id;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          textNotIn));
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
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
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

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
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

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("Unknown WHERE column: missing"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}

TEST(ExecuteQueryActionTest, FiltersTextWithLikeWildcards) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{{1, "Ada"},
                                                         {2, "Adam"},
                                                         {3, "Madam"},
                                                         {4, "Grace"},
                                                         {5, "Al"},
                                                         {6, ""}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto prefix = action.execute(
      "SELECT name FROM people WHERE name LIKE 'Ad%' ORDER BY id;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          prefix));
  const auto &prefixRows =
      std::get<QueryExecutionService::QueryExecutionResult>(prefix).rows;
  ASSERT_EQ(prefixRows.size(), 2U);
  EXPECT_EQ(std::get<std::string>(prefixRows[0][0]), "Ada");
  EXPECT_EQ(std::get<std::string>(prefixRows[1][0]), "Adam");

  const auto contains = action.execute(
      "SELECT name FROM people WHERE name LIKE '%dam%' ORDER BY id;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          contains));
  const auto &containsRows =
      std::get<QueryExecutionService::QueryExecutionResult>(contains).rows;
  ASSERT_EQ(containsRows.size(), 2U);
  EXPECT_EQ(std::get<std::string>(containsRows[0][0]), "Adam");
  EXPECT_EQ(std::get<std::string>(containsRows[1][0]), "Madam");

  const auto exact =
      action.execute("SELECT name FROM people WHERE name LIKE 'Ada';");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          exact));
  const auto &exactRows =
      std::get<QueryExecutionService::QueryExecutionResult>(exact).rows;
  ASSERT_EQ(exactRows.size(), 1U);
  EXPECT_EQ(std::get<std::string>(exactRows[0][0]), "Ada");

  const auto suffix =
      action.execute("SELECT name FROM people WHERE name LIKE '%ce';");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          suffix));
  const auto &suffixRows =
      std::get<QueryExecutionService::QueryExecutionResult>(suffix).rows;
  ASSERT_EQ(suffixRows.size(), 1U);
  EXPECT_EQ(std::get<std::string>(suffixRows[0][0]), "Grace");

  const auto oneCharacter = action.execute(
      "SELECT name FROM people WHERE name LIKE 'A_' ORDER BY id;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          oneCharacter));
  const auto &oneCharacterRows =
      std::get<QueryExecutionService::QueryExecutionResult>(oneCharacter).rows;
  ASSERT_EQ(oneCharacterRows.size(), 1U);
  EXPECT_EQ(std::get<std::string>(oneCharacterRows[0][0]), "Al");
}

TEST(ExecuteQueryActionTest, SupportsLikeEmptyAndWildcardOnlyPatterns) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, ""}, {2, "Ada"}, {3, "Grace"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto empty =
      action.execute("SELECT name FROM people WHERE name LIKE '';");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          empty));
  const auto &emptyRows =
      std::get<QueryExecutionService::QueryExecutionResult>(empty).rows;
  ASSERT_EQ(emptyRows.size(), 1U);
  EXPECT_TRUE(std::get<std::string>(emptyRows[0][0]).empty());

  const auto all =
      action.execute("SELECT name FROM people WHERE name LIKE '%';");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(all));
  EXPECT_EQ(
      std::get<QueryExecutionService::QueryExecutionResult>(all).rows.size(),
      3U);
}

TEST(ExecuteQueryActionTest, SupportsNotLikeAndNestedPredicates) {
  FakeQesRdbServicePort port;
  for (const auto &[id, name] :
       std::vector<std::pair<std::int64_t, std::string>>{
           {1, "Ada"}, {2, "Adam"}, {3, "Grace"}, {4, "Linus"}}) {
    port.rows.push_back(SqlTupleCodec::encode(
        {unidb::sql::SqlType::Int, unidb::sql::SqlType::Text}, {id, name}));
  }
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT name FROM people WHERE (name NOT LIKE 'A%' OR id = 1) "
      "AND id > 1 ORDER BY id;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome).rows;
  ASSERT_EQ(rows.size(), 2U);
  EXPECT_EQ(std::get<std::string>(rows[0][0]), "Grace");
  EXPECT_EQ(std::get<std::string>(rows[1][0]), "Linus");
}

TEST(ExecuteQueryActionTest, RejectsLikeOnIntegerColumnBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  const auto outcome =
      action.execute("SELECT * FROM people WHERE id LIKE '1%';");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("LIKE requires a TEXT column: id"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}

TEST(ExecuteQueryActionTest, RejectsNonTextLikePatternBeforeReadingRows) {
  FakeQesRdbServicePort port;
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  const auto outcome =
      action.execute("SELECT * FROM people WHERE name LIKE 1;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome)
                .message.find("WHERE value type does not match column: name"),
            std::string::npos);
  EXPECT_EQ(port.selectCalls, 0U);
}

TEST(ExecuteQueryActionTest, FiltersNullRowsUsingIsNullAndIsNotNull) {
  FakeQesRdbServicePort port;
  port.schema = {
      {"id", unidb::sql::SqlType::Int,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::BigInt, {}}},
      {"name", unidb::sql::SqlType::Text,
       unidb::sql::SqlTypeDescriptor{unidb::sql::SqlTypeId::Text, {}}}};
  port.rows.push_back(SqlTupleCodec::encode(
      port.schema, {unidb::sql::SqlNullValue{}, std::string("missing id")}));
  port.rows.push_back(SqlTupleCodec::encode(
      port.schema, {std::int64_t{2}, std::string("Ada")}));
  unidb::sql::SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto nullRows =
      action.execute("SELECT name FROM people WHERE id IS NULL;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          nullRows));
  const auto &nullResult =
      std::get<QueryExecutionService::QueryExecutionResult>(nullRows);
  ASSERT_EQ(nullResult.rows.size(), 1U);
  EXPECT_EQ(std::get<std::string>(nullResult.rows[0][0]), "missing id");

  const auto nonNullRows =
      action.execute("SELECT name FROM people WHERE id IS NOT NULL;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          nonNullRows));
  const auto &nonNullResult =
      std::get<QueryExecutionService::QueryExecutionResult>(nonNullRows);
  ASSERT_EQ(nonNullResult.rows.size(), 1U);
  EXPECT_EQ(std::get<std::string>(nonNullResult.rows[0][0]), "Ada");

  const auto comparison =
      action.execute("SELECT name FROM people WHERE id = 2;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          comparison));
  const auto &comparisonResult =
      std::get<QueryExecutionService::QueryExecutionResult>(comparison);
  ASSERT_EQ(comparisonResult.rows.size(), 1U);
  EXPECT_EQ(std::get<std::string>(comparisonResult.rows[0][0]), "Ada");

  const auto nullComparison =
      action.execute("SELECT name FROM people WHERE id = NULL;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          nullComparison));
  EXPECT_TRUE(
      std::get<QueryExecutionService::QueryExecutionResult>(nullComparison)
          .rows.empty());

  const auto unknownOrTrue =
      action.execute("SELECT name FROM people WHERE id = NULL OR id IS NULL;");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          unknownOrTrue));
  const auto &unknownOrTrueResult =
      std::get<QueryExecutionService::QueryExecutionResult>(unknownOrTrue);
  ASSERT_EQ(unknownOrTrueResult.rows.size(), 1U);
  EXPECT_EQ(std::get<std::string>(unknownOrTrueResult.rows[0][0]),
            "missing id");
}

TEST(ExecuteQueryActionTest, ExecutesInAndNotInSubqueriesWithSqlNullSemantics) {
  FakeQesRdbServicePort port;
  using namespace unidb::sql;
  const std::vector<ColumnDefinition> peopleSchema{
      {"id", SqlType::Int, SqlTypeDescriptor{SqlTypeId::BigInt, {}}},
      {"name", SqlType::Text, SqlTypeDescriptor{SqlTypeId::Text, {}}}};
  const std::vector<ColumnDefinition> ordersSchema{
      {"person_id", SqlType::Int, SqlTypeDescriptor{SqlTypeId::BigInt, {}}}};
  port.schemasByTable["people"] = peopleSchema;
  port.schemasByTable["orders"] = ordersSchema;
  port.rowsByTable["people"] = {
      SqlTupleCodec::encode(peopleSchema, {std::int64_t{1}, std::string("A")}),
      SqlTupleCodec::encode(peopleSchema, {std::int64_t{2}, std::string("B")}),
      SqlTupleCodec::encode(peopleSchema, {SqlNullValue{}, std::string("C")})};
  port.rowsByTable["orders"] = {
      SqlTupleCodec::encode(ordersSchema, {std::int64_t{2}}),
      SqlTupleCodec::encode(ordersSchema, {SqlNullValue{}})};
  SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto in = action.execute(
      "SELECT id FROM people WHERE id IN (SELECT person_id FROM orders)");
  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionResult>(in));
  const auto &inRows = std::get<QueryExecutionService::QueryExecutionResult>(in).rows;
  ASSERT_EQ(inRows.size(), 1U);
  EXPECT_EQ(std::get<std::int64_t>(inRows.front().front()), 2);

  const auto notIn = action.execute(
      "SELECT id FROM people WHERE id NOT IN (SELECT person_id FROM orders)");
  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionResult>(notIn));
  EXPECT_TRUE(std::get<QueryExecutionService::QueryExecutionResult>(notIn).rows.empty());

  const auto emptyNotIn =
      action.execute("SELECT id FROM people WHERE id NOT IN "
                     "(SELECT person_id FROM orders WHERE person_id = 999)");
  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionResult>(emptyNotIn));
  const auto &emptyNotInRows = std::get<QueryExecutionService::QueryExecutionResult>(emptyNotIn).rows;
  ASSERT_EQ(emptyNotInRows.size(), 3U);
  EXPECT_TRUE(
      std::holds_alternative<SqlNullValue>(emptyNotInRows.back().front()));

  const auto emptyIn =
      action.execute("SELECT id FROM people WHERE id IN "
                     "(SELECT person_id FROM orders WHERE person_id = 999)");
  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionResult>(emptyIn));
  EXPECT_TRUE(std::get<QueryExecutionService::QueryExecutionResult>(emptyIn).rows.empty());
}

TEST(ExecuteQueryActionTest, ExecutesEachSubqueryPredicateOncePerStatement) {
  FakeQesRdbServicePort port;
  using namespace unidb::sql;
  const std::vector<ColumnDefinition> peopleSchema{{"id", SqlType::Int}};
  const std::vector<ColumnDefinition> ordersSchema{{"person_id", SqlType::Int}};
  port.schemasByTable["people"] = peopleSchema;
  port.schemasByTable["orders"] = ordersSchema;
  port.rowsByTable["people"] = {
      SqlTupleCodec::encode(peopleSchema, {std::int64_t{1}}),
      SqlTupleCodec::encode(peopleSchema, {std::int64_t{2}})};
  port.rowsByTable["orders"] = {
      SqlTupleCodec::encode(ordersSchema, {std::int64_t{2}})};
  SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome = action.execute(
      "SELECT id FROM people WHERE id IN (SELECT person_id FROM orders) "
      "OR id NOT IN (SELECT person_id FROM orders)");
  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionResult>(outcome));
  EXPECT_EQ(port.selectCalls, 3U);
  const auto &rows = std::get<QueryExecutionService::QueryExecutionResult>(outcome).rows;
  ASSERT_EQ(rows.size(), 2U);
  EXPECT_EQ(std::get<std::int64_t>(rows[0].front()), 1);
  EXPECT_EQ(std::get<std::int64_t>(rows[1].front()), 2);
}

TEST(ExecuteQueryActionTest, RejectsMultiColumnAndIncompatibleSubqueryResults) {
  FakeQesRdbServicePort port;
  using namespace unidb::sql;
  const std::vector<ColumnDefinition> peopleSchema{{"id", SqlType::Int}};
  const std::vector<ColumnDefinition> ordersSchema{{"person_id", SqlType::Int},
                                                   {"status", SqlType::Text}};
  port.schemasByTable["people"] = peopleSchema;
  port.schemasByTable["orders"] = ordersSchema;
  port.rowsByTable["people"] = {
      SqlTupleCodec::encode(peopleSchema, {std::int64_t{1}})};
  port.rowsByTable["orders"] = {SqlTupleCodec::encode(
      ordersSchema, {std::int64_t{1}, std::string("paid")})};
  SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto multiColumn =
      action.execute("SELECT id FROM people WHERE id IN (SELECT person_id, "
                     "status FROM orders)");
  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(multiColumn));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(multiColumn)
                .message.find("exactly one column"),
            std::string::npos);

  const auto mismatch = action.execute(
      "SELECT id FROM people WHERE id IN (SELECT status FROM orders)");
  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(mismatch));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(mismatch).message.find(
                "type does not match"),
            std::string::npos);

  const auto mismatchWithEmptyResult =
      action.execute("SELECT id FROM people WHERE id IN "
                     "(SELECT status FROM orders WHERE person_id = 999)");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(mismatchWithEmptyResult));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(mismatchWithEmptyResult)
                .message.find("type does not match"),
            std::string::npos);
}

TEST(ExecuteQueryActionTest, RejectsInSubqueriesForOuterJoinQueries) {
  FakeQesRdbServicePort port;
  SqlParserImpl parser;
  auto action = makeQesAction(port, parser);
  const auto outcome = action.execute(
      "SELECT people.id FROM people INNER JOIN orders ON people.id = "
      "orders.person_id WHERE people.id IN (SELECT person_id FROM orders)");
  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome).message.find(
                "single-table SELECT"),
            std::string::npos);
}

TEST(ExecuteQueryActionTest, RejectsCorrelatedSubqueryColumnReferences) {
  FakeQesRdbServicePort port;
  using namespace unidb::sql;
  const std::vector<ColumnDefinition> peopleSchema{{"id", SqlType::Int}};
  const std::vector<ColumnDefinition> ordersSchema{{"person_id", SqlType::Int}};
  port.schemasByTable["people"] = peopleSchema;
  port.schemasByTable["orders"] = ordersSchema;
  SqlParserImpl parser;
  auto action = makeQesAction(port, parser);

  const auto outcome =
      action.execute("SELECT id FROM people p WHERE id IN "
                     "(SELECT person_id FROM orders o WHERE p.id = 1)");
  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(outcome));
  EXPECT_NE(std::get<QueryExecutionService::QueryExecutionError>(outcome).message.find(
                "Unknown WHERE table reference"),
            std::string::npos);
}
