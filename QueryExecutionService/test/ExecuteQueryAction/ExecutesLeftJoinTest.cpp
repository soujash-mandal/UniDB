#include "../../actions/ExecuteQueryAction.h"
#include "../../domain/codec/SqlTupleCodec.h"
#include "../../domain/parser/SqlParserImpl.h"
#include "../../port/RdbServicePort.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using unidb::sql::ColumnDefinition;
using unidb::sql::CreateTableStatement;
using unidb::sql::SqlNullValue;
using unidb::sql::SqlType;
using unidb::sql::SqlValue;

class LeftJoinPort final : public QueryExecutionService::RdbServicePort {
public:
  std::unordered_map<std::string, std::vector<ColumnDefinition>> schemas;
  std::unordered_map<std::string, std::vector<std::vector<char>>> rows;

  std::uint32_t createTable(const CreateTableStatement &) override { return 0; }

  std::vector<ColumnDefinition>
  getTableDefinition(const std::string &tableName) override {
    return schemas.at(tableName);
  }

  void insertRow(const std::string &, const std::vector<char> &) override {}

  std::vector<std::vector<char>>
  selectAllRows(const std::string &tableName) override {
    return rows.at(tableName);
  }
};

struct LeftJoinFixture {
  LeftJoinPort port;
  unidb::sql::SqlParserImpl parser;

  LeftJoinFixture() {
    port.schemas["users"] = {{"id", SqlType::Int}, {"name", SqlType::Text}};
    port.schemas["orders"] = {{"id", SqlType::Int},
                              {"user_id", SqlType::Int},
                              {"amount", SqlType::Int}};
    port.rows["users"] = {};
    port.rows["orders"] = {};
  }

  void addUser(SqlValue id, std::string name) {
    port.rows["users"].push_back(SqlTupleCodec::encode(
        {SqlType::Int, SqlType::Text}, {std::move(id), std::move(name)}));
  }

  void addOrder(std::int64_t id, std::int64_t userId, std::int64_t amount) {
    port.rows["orders"].push_back(SqlTupleCodec::encode(
        {SqlType::Int, SqlType::Int, SqlType::Int},
        {SqlValue{id}, SqlValue{userId}, SqlValue{amount}}));
  }

  QueryExecutionService::QueryExecutionOutcome execute(std::string_view sql) {
    QueryExecutionService::ExecuteQueryAction action(port, parser);
    return action.execute(sql);
  }
};

TEST(ExecuteQueryActionLeftJoinTest, PreservesUnmatchedLeftRowsWithNullValues) {
  LeftJoinFixture fixture;
  fixture.addUser(std::int64_t{1}, "Ada");
  fixture.addUser(std::int64_t{2}, "Lin");
  fixture.addOrder(10, 1, 250);

  const auto outcome = fixture.execute(
      "SELECT users.id, orders.amount FROM users LEFT JOIN orders "
      "ON users.id = orders.user_id");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(result.rows[0],
            (std::vector<SqlValue>{std::int64_t{1}, std::int64_t{250}}));
  EXPECT_EQ(result.rows[1],
            (std::vector<SqlValue>{std::int64_t{2}, SqlNullValue{}}));
}

TEST(ExecuteQueryActionLeftJoinTest, EmitsOneRowForEveryMatchingRightRow) {
  LeftJoinFixture fixture;
  fixture.addUser(std::int64_t{1}, "Ada");
  fixture.addOrder(10, 1, 250);
  fixture.addOrder(11, 1, 300);

  const auto outcome = fixture.execute(
      "SELECT users.name, orders.amount FROM users LEFT JOIN orders "
      "ON users.id = orders.user_id");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(result.rows[0],
            (std::vector<SqlValue>{std::string{"Ada"}, std::int64_t{250}}));
  EXPECT_EQ(result.rows[1],
            (std::vector<SqlValue>{std::string{"Ada"}, std::int64_t{300}}));
}

TEST(ExecuteQueryActionLeftJoinTest, NullJoinKeysNeverMatch) {
  LeftJoinFixture fixture;
  fixture.addUser(SqlNullValue{}, "Unknown");
  fixture.addOrder(10, 0, 250);

  const auto outcome = fixture.execute(
      "SELECT users.name, orders.amount FROM users LEFT JOIN orders "
      "ON users.id = orders.user_id");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 1U);
  EXPECT_EQ(result.rows[0],
            (std::vector<SqlValue>{std::string{"Unknown"}, SqlNullValue{}}));
}

TEST(ExecuteQueryActionLeftJoinTest, EmptyRightTableNullExtendsEveryLeftRow) {
  LeftJoinFixture fixture;
  fixture.addUser(std::int64_t{1}, "Ada");
  fixture.addUser(std::int64_t{2}, "Lin");

  const auto outcome =
      fixture.execute("SELECT * FROM users LEFT OUTER JOIN orders "
                      "ON users.id = orders.user_id");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 2U);
  ASSERT_EQ(result.rows[0].size(), 5U);
  EXPECT_TRUE(std::holds_alternative<SqlNullValue>(result.rows[0][2]));
  EXPECT_TRUE(std::holds_alternative<SqlNullValue>(result.rows[0][3]));
  EXPECT_TRUE(std::holds_alternative<SqlNullValue>(result.rows[0][4]));
  EXPECT_TRUE(std::holds_alternative<SqlNullValue>(result.rows[1][2]));
}

TEST(ExecuteQueryActionLeftJoinTest, AppliesWhereAfterNullExtension) {
  LeftJoinFixture fixture;
  fixture.addUser(std::int64_t{1}, "Ada");
  fixture.addUser(std::int64_t{2}, "Lin");
  fixture.addOrder(10, 1, 250);

  const auto unmatched = fixture.execute(
      "SELECT users.name FROM users LEFT JOIN orders "
      "ON users.id = orders.user_id WHERE orders.amount IS NULL");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          unmatched));
  const auto &unmatchedRows =
      std::get<QueryExecutionService::QueryExecutionResult>(unmatched).rows;
  ASSERT_EQ(unmatchedRows.size(), 1U);
  EXPECT_EQ(unmatchedRows[0], (std::vector<SqlValue>{std::string{"Lin"}}));

  const auto filtered =
      fixture.execute("SELECT users.name FROM users LEFT JOIN orders "
                      "ON users.id = orders.user_id WHERE orders.amount > 100");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          filtered));
  const auto &filteredRows =
      std::get<QueryExecutionService::QueryExecutionResult>(filtered).rows;
  ASSERT_EQ(filteredRows.size(), 1U);
  EXPECT_EQ(filteredRows[0], (std::vector<SqlValue>{std::string{"Ada"}}));
}

TEST(ExecuteQueryActionLeftJoinTest, SupportsAliasesAndRejectsInvalidJoinKeys) {
  LeftJoinFixture fixture;
  fixture.addUser(std::int64_t{1}, "Ada");
  fixture.addOrder(10, 1, 250);

  const auto aliased =
      fixture.execute("SELECT u.name, o.amount FROM users u LEFT JOIN orders o "
                      "ON u.id = o.user_id");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          aliased));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(aliased).rows;
  ASSERT_EQ(rows.size(), 1U);
  EXPECT_EQ(rows[0],
            (std::vector<SqlValue>{std::string{"Ada"}, std::int64_t{250}}));

  const auto invalid = fixture.execute("SELECT * FROM users LEFT JOIN orders "
                                       "ON users.id = users.id");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          invalid));
  EXPECT_EQ(
      std::get<QueryExecutionService::QueryExecutionError>(invalid).message,
      "LEFT JOIN ON must compare columns from both tables");
}

} // namespace
