#include "../../actions/ExecuteQueryAction.h"
#include "../../domain/codec/SqlTupleCodec.h"
#include "../../domain/parser/SqlParserImpl.h"
#include "../../port/RdbServicePort.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using unidb::sql::ColumnDefinition;
using unidb::sql::CreateTableStatement;
using unidb::sql::SqlNullValue;
using unidb::sql::SqlType;
using unidb::sql::SqlValue;

class FullJoinPort final : public QueryExecutionService::RdbServicePort {
public:
  std::unordered_map<std::string, std::vector<ColumnDefinition>> schemas;
  std::unordered_map<std::string, std::vector<std::vector<char>>> rows;

  std::uint32_t createTable(const CreateTableStatement &) override { return 0; }
  std::vector<ColumnDefinition>
  getTableDefinition(const std::string &name) override {
    return schemas.at(name);
  }
  void insertRow(const std::string &, const std::vector<char> &) override {}
  std::vector<std::vector<char>>
  selectAllRows(const std::string &name) override {
    return rows.at(name);
  }
};

struct FullJoinFixture {
  FullJoinPort port;
  unidb::sql::SqlParserImpl parser;

  FullJoinFixture() {
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
  void addOrder(std::int64_t id, SqlValue userId, std::int64_t amount) {
    port.rows["orders"].push_back(SqlTupleCodec::encode(
        {SqlType::Int, SqlType::Int, SqlType::Int},
        {SqlValue{id}, std::move(userId), SqlValue{amount}}));
  }
  QueryExecutionService::QueryExecutionOutcome execute(std::string_view sql) {
    QueryExecutionService::ExecuteQueryAction action(port, parser);
    return action.execute(sql);
  }
};

TEST(ExecuteQueryActionFullJoinTest,
     PreservesUnmatchedRowsFromBothTablesWithNullValues) {
  FullJoinFixture fixture;
  fixture.addUser(std::int64_t{1}, "Ada");
  fixture.addUser(std::int64_t{2}, "Lin");
  fixture.addOrder(10, std::int64_t{1}, 250);
  fixture.addOrder(11, std::int64_t{99}, 400);
  const auto outcome = fixture.execute(
      "SELECT users.id, users.name, orders.id, orders.amount FROM users "
      "FULL OUTER JOIN orders ON users.id = orders.user_id");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome).rows;
  ASSERT_EQ(rows.size(), 3U);
  EXPECT_EQ(rows[0],
            (std::vector<SqlValue>{std::int64_t{1}, std::string{"Ada"},
                                   std::int64_t{10}, std::int64_t{250}}));
  EXPECT_EQ(rows[1], (std::vector<SqlValue>{std::int64_t{2}, std::string{"Lin"},
                                            SqlNullValue{}, SqlNullValue{}}));
  EXPECT_EQ(rows[2],
            (std::vector<SqlValue>{SqlNullValue{}, SqlNullValue{},
                                   std::int64_t{11}, std::int64_t{400}}));
}

TEST(ExecuteQueryActionFullJoinTest, EmitsEveryDuplicateMatch) {
  FullJoinFixture fixture;
  fixture.addUser(std::int64_t{1}, "Ada");
  fixture.addOrder(10, std::int64_t{1}, 250);
  fixture.addOrder(11, std::int64_t{1}, 300);
  const auto outcome =
      fixture.execute("SELECT u.name, o.amount FROM users u FULL JOIN orders o "
                      "ON u.id = o.user_id");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome).rows;
  ASSERT_EQ(rows.size(), 2U);
  EXPECT_EQ(rows[0],
            (std::vector<SqlValue>{std::string{"Ada"}, std::int64_t{250}}));
  EXPECT_EQ(rows[1],
            (std::vector<SqlValue>{std::string{"Ada"}, std::int64_t{300}}));
}


TEST(ExecuteQueryActionFullJoinTest, SupportsNonEqualityJoinComparisons) {
  FullJoinFixture fixture;
  fixture.addUser(std::int64_t{1}, "Ada");
  fixture.addOrder(10, std::int64_t{2}, 250);

  const auto outcome = fixture.execute(
      "SELECT users.name, orders.amount FROM users FULL JOIN orders "
      "ON users.id != orders.user_id");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome).rows;
  ASSERT_EQ(rows.size(), 1U);
  EXPECT_EQ(rows[0],
            (std::vector<SqlValue>{std::string{"Ada"}, std::int64_t{250}}));
}

TEST(ExecuteQueryActionFullJoinTest, NullJoinKeysNeverMatch) {
  FullJoinFixture fixture;
  fixture.addUser(SqlNullValue{}, "Unknown");
  fixture.addOrder(10, SqlNullValue{}, 250);
  const auto outcome = fixture.execute(
      "SELECT users.name, orders.amount FROM users FULL JOIN orders "
      "ON users.id = orders.user_id");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome).rows;
  ASSERT_EQ(rows.size(), 2U);
  EXPECT_EQ(rows[0],
            (std::vector<SqlValue>{std::string{"Unknown"}, SqlNullValue{}}));
  EXPECT_EQ(rows[1],
            (std::vector<SqlValue>{SqlNullValue{}, std::int64_t{250}}));
}

TEST(ExecuteQueryActionFullJoinTest,
     DoesNotTurnWhereFilteredMatchesIntoUnmatchedRows) {
  FullJoinFixture fixture;
  fixture.addUser(std::int64_t{1}, "Ada");
  fixture.addOrder(10, std::int64_t{1}, 250);
  const auto outcome = fixture.execute(
      "SELECT users.name, orders.amount FROM users FULL JOIN orders "
      "ON users.id = orders.user_id WHERE orders.amount > 500");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  EXPECT_TRUE(std::get<QueryExecutionService::QueryExecutionResult>(outcome)
                  .rows.empty());
}

TEST(ExecuteQueryActionFullJoinTest, HandlesEitherTableBeingEmpty) {
  FullJoinFixture leftEmpty;
  leftEmpty.addOrder(10, std::int64_t{1}, 250);
  const auto rightOnly = leftEmpty.execute(
      "SELECT users.id, orders.amount FROM users FULL JOIN orders "
      "ON users.id = orders.user_id");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          rightOnly));
  const auto &rightRows =
      std::get<QueryExecutionService::QueryExecutionResult>(rightOnly).rows;
  ASSERT_EQ(rightRows.size(), 1U);
  EXPECT_EQ(rightRows[0],
            (std::vector<SqlValue>{SqlNullValue{}, std::int64_t{250}}));

  FullJoinFixture rightEmpty;
  rightEmpty.addUser(std::int64_t{1}, "Ada");
  const auto leftOnly = rightEmpty.execute(
      "SELECT users.id, orders.amount FROM users FULL JOIN orders "
      "ON users.id = orders.user_id");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          leftOnly));
  const auto &leftRows =
      std::get<QueryExecutionService::QueryExecutionResult>(leftOnly).rows;
  ASSERT_EQ(leftRows.size(), 1U);
  EXPECT_EQ(leftRows[0],
            (std::vector<SqlValue>{std::int64_t{1}, SqlNullValue{}}));
}

TEST(ExecuteQueryActionFullJoinTest, RejectsJoinKeysFromTheSameTable) {
  FullJoinFixture fixture;
  const auto outcome = fixture.execute(
      "SELECT * FROM users FULL JOIN orders ON users.id = users.id");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_EQ(
      std::get<QueryExecutionService::QueryExecutionError>(outcome).message,
      "FULL JOIN ON must compare columns from both tables");
}

} // namespace
