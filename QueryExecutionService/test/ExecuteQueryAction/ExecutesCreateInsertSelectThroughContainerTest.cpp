#include "../../../DatabaseStartup.h"
#include "../../../container/container.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <variant>

TEST(ExecuteQueryActionIntegrationTest,
     ExecutesCreateInsertSelectThroughContainer) {
  // Scenario 1: The normal startup and Container wiring execute the V1 path.
  const auto databaseFile =
      std::filesystem::temp_directory_path() / "unidb_qes_integration_test.db";
  std::filesystem::remove(databaseFile);

  {
    Container container(databaseFile.string(), 2, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));

    const auto created = container.executeQueryAction().execute(
        "CREATE TABLE people (id INT, name TEXT);");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            created));

    const auto inserted = container.executeQueryAction().execute(
        "INSERT INTO people (id, name) VALUES (7, 'Ada');");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            inserted));

    const auto selected =
        container.executeQueryAction().execute("SELECT * FROM people;");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            selected));
    const auto &result =
        std::get<QueryExecutionService::QueryExecutionResult>(selected);
    ASSERT_EQ(result.rows.size(), 1U);
    ASSERT_EQ(result.rows[0].size(), 2U);
    EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 7);
    EXPECT_EQ(std::get<std::string>(result.rows[0][1]), "Ada");
  }

  std::filesystem::remove(databaseFile);
}
