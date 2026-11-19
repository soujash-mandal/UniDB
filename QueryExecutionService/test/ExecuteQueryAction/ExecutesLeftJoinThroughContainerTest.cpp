#include "../../../DatabaseStartup.h"
#include "../../../container/container.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <variant>

namespace {

using QueryResult = QueryExecutionService::QueryExecutionResult;
using QueryError = QueryExecutionService::QueryExecutionError;

TEST(ExecuteQueryActionLeftJoinIntegrationTest,
     ExecutesLeftJoinThroughContainerAndNullExtendsUnmatchedRows) {
  const auto databaseFile = std::filesystem::temp_directory_path() /
                            "unidb_left_join_container_integration_test.db";
  std::filesystem::remove(databaseFile);

  {
    Container container(databaseFile.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));

    const auto usersCreated = container.executeQueryAction().execute(
        "CREATE TABLE users (id INT, name TEXT);");
    ASSERT_TRUE(std::holds_alternative<QueryResult>(usersCreated));

    const auto ordersCreated = container.executeQueryAction().execute(
        "CREATE TABLE orders (id INT, user_id INT, amount INT);");
    ASSERT_TRUE(std::holds_alternative<QueryResult>(ordersCreated));

    const auto usersInserted = container.executeQueryAction().execute(
        "INSERT INTO users (id, name) VALUES (1, 'Ada'), (2, 'Lin');");
    ASSERT_TRUE(std::holds_alternative<QueryResult>(usersInserted));

    const auto ordersInserted = container.executeQueryAction().execute(
        "INSERT INTO orders (id, user_id, amount) VALUES "
        "(10, 1, 250), (11, 1, 300);");
    ASSERT_TRUE(std::holds_alternative<QueryResult>(ordersInserted));

    const auto selected = container.executeQueryAction().execute(
        "SELECT users.id, users.name, orders.amount FROM users "
        "LEFT JOIN orders ON users.id = orders.user_id;");
    ASSERT_TRUE(std::holds_alternative<QueryResult>(selected))
        << std::get<QueryError>(selected).message;

    const auto &result = std::get<QueryResult>(selected);
    ASSERT_EQ(result.rows.size(), 3U);
    EXPECT_EQ(result.rows[0][0], std::int64_t{1});
    EXPECT_EQ(result.rows[0][1], std::string{"Ada"});
    EXPECT_EQ(result.rows[0][2], std::int64_t{250});
    EXPECT_EQ(result.rows[1][0], std::int64_t{1});
    EXPECT_EQ(result.rows[1][1], std::string{"Ada"});
    EXPECT_EQ(result.rows[1][2], std::int64_t{300});
    EXPECT_EQ(result.rows[2][0], std::int64_t{2});
    EXPECT_EQ(result.rows[2][1], std::string{"Lin"});
    EXPECT_TRUE(
        std::holds_alternative<unidb::sql::SqlNullValue>(result.rows[2][2]));

    const auto unmatched = container.executeQueryAction().execute(
        "SELECT users.name FROM users LEFT JOIN orders "
        "ON users.id = orders.user_id WHERE orders.amount IS NULL;");
    ASSERT_TRUE(std::holds_alternative<QueryResult>(unmatched))
        << std::get<QueryError>(unmatched).message;
    const auto &unmatchedRows = std::get<QueryResult>(unmatched).rows;
    ASSERT_EQ(unmatchedRows.size(), 1U);
    EXPECT_EQ(unmatchedRows[0][0], std::string{"Lin"});

    container.flushDirtyPages();
  }

  {
    Container reopened(databaseFile.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_FALSE(DatabaseStartup::execute(reopened));

    const auto selected = reopened.executeQueryAction().execute(
        "SELECT users.name, orders.amount FROM users "
        "LEFT OUTER JOIN orders ON users.id = orders.user_id;");
    ASSERT_TRUE(std::holds_alternative<QueryResult>(selected))
        << std::get<QueryError>(selected).message;

    const auto &rows = std::get<QueryResult>(selected).rows;
    ASSERT_EQ(rows.size(), 3U);
    EXPECT_TRUE(std::holds_alternative<unidb::sql::SqlNullValue>(rows[2][1]));
  }

  std::filesystem::remove(databaseFile);
}

} // namespace
