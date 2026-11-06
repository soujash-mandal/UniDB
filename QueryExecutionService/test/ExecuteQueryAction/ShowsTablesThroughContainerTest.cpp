#include "../../../DatabaseStartup.h"
#include "../../../container/container.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <variant>
TEST(ExecuteQueryActionIntegrationTest, ShowsTablesInCatalogOrderAndAfterRestart) {
  const auto path = std::filesystem::temp_directory_path() / "unidb_show_tables_test.db";
  std::filesystem::remove(path);
  {
    Container container(path.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));
    ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionResult>(container.executeQueryAction().execute("CREATE TABLE people (id INT, name TEXT);")));
    ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionResult>(container.executeQueryAction().execute("CREATE TABLE products (id INT, title TEXT);")));
    const auto outcome = container.executeQueryAction().execute("SHOW TABLES;");
    ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionResult>(outcome));
    const auto &result = std::get<QueryExecutionService::QueryExecutionResult>(outcome);
    ASSERT_EQ(result.kind, QueryExecutionService::QueryExecutionResultKind::Rows);
    ASSERT_EQ(result.columns.size(), 1U);
    EXPECT_EQ(result.columns[0].name, "table_name");
    EXPECT_EQ(result.columns[0].type, unidb::sql::SqlType::Text);
    ASSERT_EQ(result.rows.size(), 2U);
    EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "people");
    EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "products");
    container.flushDirtyPages();
  }
  {
    Container reopened(path.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_FALSE(DatabaseStartup::execute(reopened));
    const auto outcome = reopened.executeQueryAction().execute("show tables");
    ASSERT_TRUE(std::holds_alternative<QueryExecutionService::QueryExecutionResult>(outcome));
    const auto &result = std::get<QueryExecutionService::QueryExecutionResult>(outcome);
    ASSERT_EQ(result.rows.size(), 2U);
    EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "people");
    EXPECT_EQ(std::get<std::string>(result.rows[1][0]), "products");
  }
  std::filesystem::remove(path);
}
