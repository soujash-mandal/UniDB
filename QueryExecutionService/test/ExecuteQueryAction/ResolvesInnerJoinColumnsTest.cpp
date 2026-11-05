#include "../../actions/ExecuteQueryAction.h"
#include "../../port/RdbServicePort.h"
#include "../../../SQLParser/SqlParserImpl.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

using unidb::sql::ColumnDefinition;
using unidb::sql::CreateTableStatement;
using unidb::sql::SqlType;

class JoinResolutionPort final : public QueryExecutionService::RdbServicePort {
public:
  std::unordered_map<std::string, std::vector<ColumnDefinition>> schemas;
  std::vector<std::string> schemaRequests;
  std::uint32_t selectCalls{0};

  std::uint32_t
  createTable(const CreateTableStatement &) override {
    return 0;
  }

  std::vector<ColumnDefinition>
  getTableDefinition(const std::string &tableName) override {
    schemaRequests.push_back(tableName);
    const auto found = schemas.find(tableName);
    if (found == schemas.end()) {
      throw std::runtime_error("Unknown table: " + tableName);
    }
    return found->second;
  }

  void insertRow(const std::string &, const std::vector<char> &) override {}

  std::vector<std::vector<char>>
  selectAllRows(const std::string &) override {
    ++selectCalls;
    return {};
  }
};

struct JoinResolutionFixture {
  JoinResolutionPort port;
  unidb::sql::SqlParserImpl parser;

  JoinResolutionFixture() {
    port.schemas["users"] = {
        {"id", SqlType::Int}, {"name", SqlType::Text}};
    port.schemas["orders"] = {
        {"id", SqlType::Int}, {"user_id", SqlType::Int},
        {"amount", SqlType::Int}};
  }

  QueryExecutionService::QueryExecutionOutcome execute(std::string_view sql) {
    QueryExecutionService::ExecuteQueryAction action(port, parser);
    return action.execute(sql);
  }
};

TEST(ExecuteQueryActionJoinResolutionTest,
     ResolvesQualifiedColumnsAndValidatesBothJoinSchemas) {
  JoinResolutionFixture fixture;

  const auto result = fixture.execute(
      "SELECT users.id, orders.amount FROM users INNER JOIN orders "
      "ON users.id = orders.user_id");

  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(
      result));
  const auto &error =
      std::get<QueryExecutionService::QueryExecutionError>(result);
  EXPECT_EQ(error.code,
            QueryExecutionService::QueryExecutionErrorCode::UnsupportedStatement);
  EXPECT_EQ(error.message, "INNER JOIN execution is not available yet");
  EXPECT_EQ(fixture.port.schemaRequests,
            (std::vector<std::string>{"users", "orders"}));
  EXPECT_EQ(fixture.port.selectCalls, 0U);
}

TEST(ExecuteQueryActionJoinResolutionTest,
     AcceptsUnambiguousUnqualifiedSelectColumns) {
  JoinResolutionFixture fixture;

  const auto result = fixture.execute(
      "SELECT name, amount FROM users INNER JOIN orders "
      "ON users.id = orders.user_id");

  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(
      result));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(result).message,
            "INNER JOIN execution is not available yet");
}

TEST(ExecuteQueryActionJoinResolutionTest, RejectsAmbiguousSelectColumns) {
  JoinResolutionFixture fixture;

  const auto result = fixture.execute(
      "SELECT id FROM users INNER JOIN orders "
      "ON users.id = orders.user_id");

  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(
      result));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(result).code,
            QueryExecutionService::QueryExecutionErrorCode::ExecutionError);
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(result).message,
            "Ambiguous SELECT column: id");
}

TEST(ExecuteQueryActionJoinResolutionTest, RejectsUnknownQualifiedSelectColumn) {
  JoinResolutionFixture fixture;

  const auto result = fixture.execute(
      "SELECT payments.amount FROM users INNER JOIN orders "
      "ON users.id = orders.user_id");

  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(
      result));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(result).message,
            "Unknown SELECT table: payments");
}

TEST(ExecuteQueryActionJoinResolutionTest, RejectsUnknownJoinColumn) {
  JoinResolutionFixture fixture;

  const auto result = fixture.execute(
      "SELECT users.name FROM users INNER JOIN orders "
      "ON users.missing = orders.user_id");

  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(
      result));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(result).message,
            "Unknown JOIN column: users.missing");
}

TEST(ExecuteQueryActionJoinResolutionTest,
     RejectsJoinConditionThatReferencesOnlyOneTable) {
  JoinResolutionFixture fixture;

  const auto result = fixture.execute(
      "SELECT users.name FROM users INNER JOIN orders "
      "ON users.id = users.id");

  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(
      result));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(result).message,
            "INNER JOIN ON must compare columns from both tables");
}

TEST(ExecuteQueryActionJoinResolutionTest, RejectsMismatchedJoinKeyTypes) {
  JoinResolutionFixture fixture;
  fixture.port.schemas["orders"][1].type = SqlType::Text;

  const auto result = fixture.execute(
      "SELECT users.name FROM users INNER JOIN orders "
      "ON users.id = orders.user_id");

  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(
      result));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(result).message,
            "INNER JOIN columns must have matching types");
}

TEST(ExecuteQueryActionJoinResolutionTest, RejectsDuplicateOutputAliases) {
  JoinResolutionFixture fixture;

  const auto result = fixture.execute(
      "SELECT users.id AS value, orders.amount AS value FROM users "
      "INNER JOIN orders ON users.id = orders.user_id");

  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(
      result));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(result).message,
            "Duplicate SELECT output column name: value");
}

TEST(ExecuteQueryActionJoinResolutionTest, SelectStarResolvesDuplicateNames) {
  JoinResolutionFixture fixture;

  const auto result = fixture.execute(
      "SELECT * FROM users INNER JOIN orders "
      "ON users.id = orders.user_id");

  ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionError>(
      result));
  EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionError>(result).message,
            "INNER JOIN execution is not available yet");
}

} // namespace
