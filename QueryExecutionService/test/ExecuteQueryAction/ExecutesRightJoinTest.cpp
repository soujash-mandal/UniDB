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

class RightJoinPort final : public QueryExecutionService::RdbServicePort {
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

struct RightJoinFixture {
  RightJoinPort port;
  unidb::sql::SqlParserImpl parser;

  RightJoinFixture() {
    port.schemas["users"] = {{"id", SqlType::Int}, {"name", SqlType::Text}};
    port.schemas["orders"] = {{"id", SqlType::Int},
                              {"user_id", SqlType::Int},
                              {"amount", SqlType::Int}};
    port.rows["users"] = {};
    port.rows["orders"] = {};
  }

  void addUser(std::int64_t id, std::string name) {
    port.rows["users"].push_back(SqlTupleCodec::encode(
        {SqlType::Int, SqlType::Text}, {SqlValue{id}, std::move(name)}));
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

TEST(ExecuteQueryActionRightJoinTest,
     PreservesUnmatchedRightRowsWithNullValues) {
  RightJoinFixture fixture;
  fixture.addUser(1, "Ada");
  fixture.addOrder(10, std::int64_t{1}, 250);
  fixture.addOrder(12, std::int64_t{99}, 400);

  const auto outcome = fixture.execute(
      "SELECT users.id, users.name, orders.amount FROM users RIGHT JOIN orders "
      "ON users.id = orders.user_id");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(result.rows[0],
            (std::vector<SqlValue>{std::int64_t{1}, std::string{"Ada"},
                                   std::int64_t{250}}));
  EXPECT_TRUE(std::holds_alternative<SqlNullValue>(result.rows[1][0]));
  EXPECT_TRUE(std::holds_alternative<SqlNullValue>(result.rows[1][1]));
  EXPECT_EQ(result.rows[1][2], std::int64_t{400});
}

TEST(ExecuteQueryActionRightJoinTest,
     EmitsEveryMatchAndKeepsLeftThenRightColumnOrder) {
  RightJoinFixture fixture;
  fixture.addUser(1, "Ada");
  fixture.addUser(2, "Lin");
  fixture.addOrder(10, std::int64_t{1}, 250);
  fixture.addOrder(11, std::int64_t{1}, 300);

  const auto outcome = fixture.execute(
      "SELECT users.name, orders.amount FROM users RIGHT OUTER JOIN orders "
      "ON users.id = orders.user_id");
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

TEST(ExecuteQueryActionRightJoinTest,
     NullJoinKeysNeverMatchAndWhereRunsAfterExtension) {
  RightJoinFixture fixture;
  fixture.addUser(1, "Ada");
  fixture.addOrder(10, SqlNullValue{}, 250);
  fixture.addOrder(11, std::int64_t{1}, 300);

  const auto outcome = fixture.execute(
      "SELECT users.name, orders.amount FROM users RIGHT JOIN orders "
      "ON users.id = orders.user_id WHERE users.name IS NULL");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome).rows;
  ASSERT_EQ(rows.size(), 1U);
  EXPECT_TRUE(std::holds_alternative<SqlNullValue>(rows[0][0]));
  EXPECT_EQ(rows[0][1], std::int64_t{250});
}

TEST(ExecuteQueryActionRightJoinTest, EmptyLeftTableNullExtendsEveryRightRow) {
  RightJoinFixture fixture;
  fixture.addOrder(10, std::int64_t{1}, 250);
  fixture.addOrder(11, std::int64_t{2}, 300);

  const auto outcome = fixture.execute(
      "SELECT * FROM users RIGHT JOIN orders ON users.id = orders.user_id");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &rows =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome).rows;
  ASSERT_EQ(rows.size(), 2U);
  ASSERT_EQ(rows[0].size(), 5U);
  EXPECT_TRUE(std::holds_alternative<SqlNullValue>(rows[0][0]));
  EXPECT_TRUE(std::holds_alternative<SqlNullValue>(rows[0][1]));
  EXPECT_EQ(rows[0][2], std::int64_t{10});
}

} // namespace
