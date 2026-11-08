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
using unidb::sql::SqlType;
using unidb::sql::SqlValue;

class JoinExecutionPort final : public QueryExecutionService::RdbServicePort {
public:
  std::unordered_map<std::string, std::vector<ColumnDefinition>> schemas;
  std::unordered_map<std::string, std::vector<std::vector<char>>> rows;
  std::vector<std::string> schemaRequests;
  std::vector<std::string> rowRequests;

  std::uint32_t createTable(const CreateTableStatement &) override { return 0; }

  std::vector<ColumnDefinition>
  getTableDefinition(const std::string &tableName) override {
    schemaRequests.push_back(tableName);
    return schemas.at(tableName);
  }

  void insertRow(const std::string &, const std::vector<char> &) override {}

  std::vector<std::vector<char>>
  selectAllRows(const std::string &tableName) override {
    rowRequests.push_back(tableName);
    return rows.at(tableName);
  }
};

struct InnerJoinFixture {
  JoinExecutionPort port;
  unidb::sql::SqlParserImpl parser;

  InnerJoinFixture() {
    port.schemas["users"] = {{"id", SqlType::Int}, {"name", SqlType::Text}};
    port.schemas["orders"] = {{"id", SqlType::Int},
                              {"user_id", SqlType::Int},
                              {"amount", SqlType::Int}};
    port.schemas["products"] = {{"order_id", SqlType::Int},
                                {"name", SqlType::Text}};
    port.rows["users"] = {};
    port.rows["orders"] = {};
    port.rows["products"] = {};
  }

  void addUser(std::int64_t id, std::string name) {
    port.rows["users"].push_back(
        SqlTupleCodec::encode({SqlType::Int, SqlType::Text},
                              {SqlValue{id}, SqlValue{std::move(name)}}));
  }

  void addOrder(std::int64_t id, std::int64_t userId, std::int64_t amount) {
    port.rows["orders"].push_back(SqlTupleCodec::encode(
        {SqlType::Int, SqlType::Int, SqlType::Int},
        {SqlValue{id}, SqlValue{userId}, SqlValue{amount}}));
  }

  void addProduct(std::int64_t orderId, std::string name) {
    port.rows["products"].push_back(
        SqlTupleCodec::encode({SqlType::Int, SqlType::Text},
                              {SqlValue{orderId}, SqlValue{std::move(name)}}));
  }

  QueryExecutionService::QueryExecutionOutcome execute(std::string_view sql) {
    QueryExecutionService::ExecuteQueryAction action(port, parser);
    return action.execute(sql);
  }
};

TEST(ExecuteQueryActionInnerJoinTest,
     JoinsMatchingRowsAndBuildsQualifiedMetadata) {
  InnerJoinFixture fixture;
  fixture.addUser(1, "Ada");
  fixture.addUser(1, "Ada duplicate key");
  fixture.addUser(2, "Lin");
  fixture.addOrder(10, 1, 250);
  fixture.addOrder(11, 1, 300);
  fixture.addOrder(12, 3, 400);

  const auto outcome = fixture.execute("SELECT * FROM users INNER JOIN orders "
                                       "ON users.id = orders.user_id");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.kind, QueryExecutionService::QueryExecutionResultKind::Rows);
  ASSERT_EQ(result.columns.size(), 5U);
  EXPECT_EQ(result.columns[0].name, "users.id");
  EXPECT_EQ(result.columns[1].name, "name");
  EXPECT_EQ(result.columns[2].name, "orders.id");
  EXPECT_EQ(result.columns[3].name, "user_id");
  EXPECT_EQ(result.columns[4].name, "amount");
  ASSERT_EQ(result.rows.size(), 4U);
  EXPECT_EQ(result.rows[0],
            (std::vector<SqlValue>{std::int64_t{1}, std::string{"Ada"},
                                   std::int64_t{10}, std::int64_t{1},
                                   std::int64_t{250}}));
  EXPECT_EQ(result.rows[1],
            (std::vector<SqlValue>{std::int64_t{1}, std::string{"Ada"},
                                   std::int64_t{11}, std::int64_t{1},
                                   std::int64_t{300}}));
  EXPECT_EQ(result.rows[2],
            (std::vector<SqlValue>{
                std::int64_t{1}, std::string{"Ada duplicate key"},
                std::int64_t{10}, std::int64_t{1}, std::int64_t{250}}));
  EXPECT_EQ(result.rows[3],
            (std::vector<SqlValue>{
                std::int64_t{1}, std::string{"Ada duplicate key"},
                std::int64_t{11}, std::int64_t{1}, std::int64_t{300}}));
  EXPECT_EQ(fixture.port.schemaRequests,
            (std::vector<std::string>{"users", "orders"}));
  EXPECT_EQ(fixture.port.rowRequests,
            (std::vector<std::string>{"users", "orders"}));
}

TEST(ExecuteQueryActionInnerJoinTest, ReturnsNoRowsWhenNoKeysMatch) {
  InnerJoinFixture fixture;
  fixture.addUser(1, "Ada");
  fixture.addOrder(10, 2, 250);

  const auto outcome = fixture.execute("SELECT * FROM users INNER JOIN orders "
                                       "ON users.id = orders.user_id");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  EXPECT_TRUE(std::get<QueryExecutionService::QueryExecutionResult>(outcome)
                  .rows.empty());
}

TEST(ExecuteQueryActionInnerJoinTest, ReturnsNoRowsWhenLeftTableIsEmpty) {
  InnerJoinFixture fixture;
  fixture.addOrder(10, 1, 250);

  const auto outcome = fixture.execute("SELECT * FROM users INNER JOIN orders "
                                       "ON users.id = orders.user_id");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  EXPECT_TRUE(std::get<QueryExecutionService::QueryExecutionResult>(outcome)
                  .rows.empty());
}

TEST(ExecuteQueryActionInnerJoinTest, ReturnsNoRowsWhenRightTableIsEmpty) {
  InnerJoinFixture fixture;
  fixture.addUser(1, "Ada");

  const auto outcome = fixture.execute("SELECT * FROM users INNER JOIN orders "
                                       "ON users.id = orders.user_id");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  EXPECT_TRUE(std::get<QueryExecutionService::QueryExecutionResult>(outcome)
                  .rows.empty());
}

TEST(ExecuteQueryActionInnerJoinTest, ProjectsQualifiedAndUnqualifiedColumns) {
  InnerJoinFixture fixture;
  fixture.addUser(1, "Ada");
  fixture.addUser(2, "Lin");
  fixture.addOrder(10, 1, 250);
  fixture.addOrder(11, 2, 80);

  const auto outcome =
      fixture.execute("SELECT users.name, amount AS total FROM users "
                      "INNER JOIN orders ON users.id = orders.user_id");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 2U);
  EXPECT_EQ(result.columns[0].name, "name");
  EXPECT_EQ(result.columns[1].name, "total");
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(result.rows[0],
            (std::vector<SqlValue>{std::string{"Ada"}, std::int64_t{250}}));
  EXPECT_EQ(result.rows[1],
            (std::vector<SqlValue>{std::string{"Lin"}, std::int64_t{80}}));
}

TEST(ExecuteQueryActionInnerJoinTest,
     FiltersJoinedRowsWithQualifiedWhereColumns) {
  InnerJoinFixture fixture;
  fixture.addUser(1, "Ada");
  fixture.addUser(2, "Lin");
  fixture.addOrder(10, 1, 250);
  fixture.addOrder(11, 2, 80);

  const auto outcome =
      fixture.execute("SELECT users.name, orders.amount FROM users "
                      "INNER JOIN orders ON users.id = orders.user_id "
                      "WHERE orders.amount > 100");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 1U);
  EXPECT_EQ(result.rows[0],
            (std::vector<SqlValue>{std::string{"Ada"}, std::int64_t{250}}));
}

TEST(ExecuteQueryActionInnerJoinTest, SupportsNonEqualityJoinPredicates) {
  InnerJoinFixture fixture;
  fixture.addUser(1, "Ada");
  fixture.addUser(2, "Lin");
  fixture.addUser(3, "Mina");
  fixture.addOrder(10, 1, 250);
  fixture.addOrder(11, 2, 80);
  fixture.addOrder(12, 3, 400);

  const auto notEqual = fixture.execute(
      "SELECT users.id, orders.user_id FROM users INNER JOIN orders "
      "ON users.id != orders.user_id");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          notEqual));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(notEqual);
  ASSERT_EQ(result.rows.size(), 6U);

  const auto greater = fixture.execute(
      "SELECT users.id, orders.user_id FROM users INNER JOIN orders "
      "ON users.id > orders.user_id");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          greater));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionResult>(greater)
                .rows.size(),
            3U);
}

TEST(ExecuteQueryActionInnerJoinTest, RejectsAmbiguousUnqualifiedWhereColumn) {
  InnerJoinFixture fixture;
  const auto outcome =
      fixture.execute("SELECT * FROM users INNER JOIN orders "
                      "ON users.id = orders.user_id WHERE id = 1");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_EQ(
      std::get<QueryExecutionService::QueryExecutionError>(outcome).message,
      "Ambiguous WHERE column: id");
}

TEST(ExecuteQueryActionInnerJoinTest,
     KeepsQualifiedWhereReferencesWorkingForSingleTableSelects) {
  InnerJoinFixture fixture;
  fixture.addUser(1, "Ada");
  fixture.addUser(2, "Lin");

  const auto outcome =
      fixture.execute("SELECT name FROM users WHERE users.id = 1");

  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.rows.size(), 1U);
  EXPECT_EQ(result.rows[0], (std::vector<SqlValue>{std::string{"Ada"}}));
}

TEST(ExecuteQueryActionInnerJoinTest, ExecutesMultipleInnerJoinsLeftToRight) {
  InnerJoinFixture fixture;
  fixture.addUser(1, "Ada");
  fixture.addUser(2, "Lin");
  fixture.addOrder(10, 1, 250);
  fixture.addOrder(11, 2, 80);
  fixture.addProduct(10, "Book");
  fixture.addProduct(11, "Pen");

  const auto outcome =
      fixture.execute("SELECT users.name, products.name FROM users "
                      "INNER JOIN orders ON users.id = orders.user_id "
                      "INNER JOIN products ON orders.id = products.order_id");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 2U);
  EXPECT_EQ(result.columns[0].name, "users.name");
  EXPECT_EQ(result.columns[1].name, "products.name");
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(result.rows[0],
            (std::vector<SqlValue>{std::string{"Ada"}, std::string{"Book"}}));
  EXPECT_EQ(result.rows[1],
            (std::vector<SqlValue>{std::string{"Lin"}, std::string{"Pen"}}));
  EXPECT_EQ(fixture.port.schemaRequests,
            (std::vector<std::string>{"users", "orders", "products"}));
  EXPECT_EQ(fixture.port.rowRequests,
            (std::vector<std::string>{"users", "orders", "products"}));
}

TEST(ExecuteQueryActionCrossJoinTest, CartesianProductOrder) {
  InnerJoinFixture fixture;
  fixture.addUser(1, "Ada");
  fixture.addUser(2, "Lin");
  fixture.addOrder(10, 1, 250);
  fixture.addOrder(11, 2, 80);

  const auto outcome = fixture.execute("SELECT * FROM users CROSS JOIN orders");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 5U);
  EXPECT_EQ(result.columns[0].name, "users.id");
  EXPECT_EQ(result.columns[1].name, "name");
  EXPECT_EQ(result.columns[2].name, "orders.id");
  EXPECT_EQ(result.columns[3].name, "user_id");
  EXPECT_EQ(result.columns[4].name, "amount");
  ASSERT_EQ(result.rows.size(), 4U);
  EXPECT_EQ(result.rows[0],
            (std::vector<SqlValue>{std::int64_t{1}, std::string{"Ada"},
                                   std::int64_t{10}, std::int64_t{1},
                                   std::int64_t{250}}));
  EXPECT_EQ(result.rows[1],
            (std::vector<SqlValue>{std::int64_t{1}, std::string{"Ada"},
                                   std::int64_t{11}, std::int64_t{2},
                                   std::int64_t{80}}));
  EXPECT_EQ(result.rows[2],
            (std::vector<SqlValue>{std::int64_t{2}, std::string{"Lin"},
                                   std::int64_t{10}, std::int64_t{1},
                                   std::int64_t{250}}));
  EXPECT_EQ(result.rows[3],
            (std::vector<SqlValue>{std::int64_t{2}, std::string{"Lin"},
                                   std::int64_t{11}, std::int64_t{2},
                                   std::int64_t{80}}));
  EXPECT_EQ(fixture.port.rowRequests,
            (std::vector<std::string>{"users", "orders"}));
}

TEST(ExecuteQueryActionCrossJoinTest, ProjectionAliasesAndWhere) {
  InnerJoinFixture fixture;
  fixture.addUser(1, "Ada");
  fixture.addUser(2, "Lin");
  fixture.addOrder(10, 1, 250);
  fixture.addOrder(11, 2, 80);

  const auto outcome =
      fixture.execute("SELECT u.name AS person, o.amount AS total FROM users "
                      "u CROSS JOIN orders o WHERE u.id = 2 AND o.amount > 50");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  const auto &result =
      std::get<QueryExecutionService::QueryExecutionResult>(outcome);
  ASSERT_EQ(result.columns.size(), 2U);
  EXPECT_EQ(result.columns[0].name, "person");
  EXPECT_EQ(result.columns[1].name, "total");
  ASSERT_EQ(result.rows.size(), 2U);
  EXPECT_EQ(result.rows[0],
            (std::vector<SqlValue>{std::string{"Lin"}, std::int64_t{250}}));
  EXPECT_EQ(result.rows[1],
            (std::vector<SqlValue>{std::string{"Lin"}, std::int64_t{80}}));
}

TEST(ExecuteQueryActionCrossJoinTest, EmptyInputProducesNoRows) {
  InnerJoinFixture fixture;
  fixture.addUser(1, "Ada");

  const auto outcome = fixture.execute("SELECT * FROM users CROSS JOIN orders");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
          outcome));
  EXPECT_TRUE(std::get<QueryExecutionService::QueryExecutionResult>(outcome)
                  .rows.empty());
}

TEST(ExecuteQueryActionCrossJoinTest, RejectsProductsAboveSafetyLimit) {
  InnerJoinFixture fixture;
  for (std::int64_t id = 0; id < 1001; ++id) {
    fixture.addUser(id, "user");
  }
  for (std::int64_t id = 0; id < 1000; ++id) {
    fixture.addOrder(id, id, 1);
  }

  const auto outcome = fixture.execute("SELECT * FROM users CROSS JOIN orders");
  ASSERT_TRUE(
      std::holds_alternative<QueryExecutionService::QueryExecutionError>(
          outcome));
  EXPECT_EQ(
      std::get<QueryExecutionService::QueryExecutionError>(outcome).message,
      "CROSS JOIN result exceeds the 1000000-row safety limit");
}

} // namespace
