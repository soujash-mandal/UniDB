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

TEST(ExecuteQueryActionRightJoinIntegrationTest,
     ExecutesRightJoinThroughContainerAndNullExtendsUnmatchedRows) {
  const auto databaseFile = std::filesystem::temp_directory_path() /
                            "unidb_right_join_container_integration_test.db";
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
        "INSERT INTO users (id, name) VALUES (1, 'Ada');");
    ASSERT_TRUE(std::holds_alternative<QueryResult>(usersInserted));

    const auto ordersInserted = container.executeQueryAction().execute(
        "INSERT INTO orders (id, user_id, amount) VALUES "
        "(10, 1, 250), (11, 1, 300), (12, 99, 400);");
    ASSERT_TRUE(std::holds_alternative<QueryResult>(ordersInserted));

    const auto selected = container.executeQueryAction().execute(
        "SELECT users.id, users.name, orders.amount FROM users "
        "RIGHT JOIN orders ON users.id = orders.user_id;");
    ASSERT_TRUE(std::holds_alternative<QueryResult>(selected))
        << std::get<QueryError>(selected).message;

    const auto &result = std::get<QueryResult>(selected);
    ASSERT_EQ(result.rows.size(), 3U);
    EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 1);
    EXPECT_EQ(std::get<std::string>(result.rows[0][1]), "Ada");
    EXPECT_EQ(std::get<std::int64_t>(result.rows[0][2]), 250);
    EXPECT_EQ(std::get<std::int64_t>(result.rows[1][0]), 1);
    EXPECT_EQ(std::get<std::string>(result.rows[1][1]), "Ada");
    EXPECT_EQ(std::get<std::int64_t>(result.rows[1][2]), 300);
    EXPECT_TRUE(
        std::holds_alternative<unidb::sql::SqlNullValue>(result.rows[2][0]));
    EXPECT_TRUE(
        std::holds_alternative<unidb::sql::SqlNullValue>(result.rows[2][1]));
    EXPECT_EQ(std::get<std::int64_t>(result.rows[2][2]), 400);

    const auto unmatched = container.executeQueryAction().execute(
        "SELECT users.name FROM users RIGHT JOIN orders "
        "ON users.id = orders.user_id WHERE users.name IS NULL;");
    ASSERT_TRUE(std::holds_alternative<QueryResult>(unmatched))
        << std::get<QueryError>(unmatched).message;
    const auto &unmatchedRows = std::get<QueryResult>(unmatched).rows;
    ASSERT_EQ(unmatchedRows.size(), 1U);
    EXPECT_TRUE(
        std::holds_alternative<unidb::sql::SqlNullValue>(unmatchedRows[0][0]));

    container.flushDirtyPages();
  }

  {
    Container reopened(databaseFile.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_FALSE(DatabaseStartup::execute(reopened));

    const auto selected = reopened.executeQueryAction().execute(
        "SELECT users.name, orders.amount FROM users "
        "RIGHT OUTER JOIN orders ON users.id = orders.user_id;");
    ASSERT_TRUE(std::holds_alternative<QueryResult>(selected))
        << std::get<QueryError>(selected).message;

    const auto &rows = std::get<QueryResult>(selected).rows;
    ASSERT_EQ(rows.size(), 3U);
    EXPECT_TRUE(std::holds_alternative<unidb::sql::SqlNullValue>(rows[2][0]));
  }

  std::filesystem::remove(databaseFile);
}

} // namespace
