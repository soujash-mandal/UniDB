#include "../../../DatabaseStartup.h"
#include "../../../container/container.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <limits>
#include <string>
#include <variant>

TEST(ExecuteQueryActionIntegrationTest,
     ExecutesCreateInsertSelectThroughContainer) {
  // Scenario 1: The normal startup and Container wiring execute the V1 path.
  const auto databaseFile =
      std::filesystem::temp_directory_path() / "unidb_qes_integration_test.db";
  std::filesystem::remove(databaseFile);

  {
    Container container(databaseFile.string(), 4, EvictionPolicyType::FIFO);
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

    // Scenario 2: Dirty pages are durable after an explicit shutdown flush.
    container.flushDirtyPages();
  }

  {
    Container reopened(databaseFile.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_FALSE(DatabaseStartup::execute(reopened));
    const auto selected =
        reopened.executeQueryAction().execute("SELECT * FROM people;");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            selected));
    const auto &result =
        std::get<QueryExecutionService::QueryExecutionResult>(selected);
    ASSERT_EQ(result.rows.size(), 1U);
    EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), 7);
    EXPECT_EQ(std::get<std::string>(result.rows[0][1]), "Ada");
  }

  std::filesystem::remove(databaseFile);
}

TEST(ExecuteQueryActionIntegrationTest,
     StoresAndReadsPostgreSqlIntegerWidthsEndToEnd) {
  const auto databaseFile =
      std::filesystem::temp_directory_path() / "unidb_integer_widths_test.db";
  std::filesystem::remove(databaseFile);

  {
    Container container(databaseFile.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));

    const auto created = container.executeQueryAction().execute(
        "CREATE TABLE integer_widths (small_value SMALLINT, "
        "integer_value INTEGER, big_value BIGINT);");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            created));

    const auto inserted = container.executeQueryAction().execute(
        "INSERT INTO integer_widths (small_value, integer_value, big_value) "
        "VALUES (-32768, 2147483647, -9223372036854775808);");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            inserted));

    const auto selected =
        container.executeQueryAction().execute("SELECT * FROM integer_widths;");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            selected));
    const auto &result =
        std::get<QueryExecutionService::QueryExecutionResult>(selected);
    ASSERT_EQ(result.rows.size(), 1U);
    ASSERT_EQ(result.rows[0].size(), 3U);
    EXPECT_EQ(std::get<std::int64_t>(result.rows[0][0]), -32768);
    EXPECT_EQ(std::get<std::int64_t>(result.rows[0][1]), 2147483647);
    EXPECT_EQ(std::get<std::int64_t>(result.rows[0][2]),
              std::numeric_limits<std::int64_t>::min());

    const auto overflow = container.executeQueryAction().execute(
        "INSERT INTO integer_widths (small_value, integer_value, big_value) "
        "VALUES (32768, 1, 1);");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionError>(
            overflow));
    const auto afterOverflow =
        container.executeQueryAction().execute("SELECT * FROM integer_widths;");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            afterOverflow));
    EXPECT_EQ(
        std::get<QueryExecutionService::QueryExecutionResult>(afterOverflow)
            .rows.size(),
        1U);
    container.flushDirtyPages();
  }

  std::filesystem::remove(databaseFile);
}

TEST(ExecuteQueryActionIntegrationTest,
     CreatesAndInsertsBooleanColumnsAcrossContainerRestart) {
  const auto databaseFile = std::filesystem::temp_directory_path() /
                            "unidb_boolean_schema_integration_test.db";
  std::filesystem::remove(databaseFile);

  {
    Container container(databaseFile.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));

    const auto created = container.executeQueryAction().execute(
        "CREATE TABLE flags (enabled BOOLEAN, visible BOOL);");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            created));

    const auto inserted = container.executeQueryAction().execute(
        "INSERT INTO flags (enabled, visible) VALUES (TRUE, FALSE), "
        "(false, true);");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            inserted));
    EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionResult>(inserted)
                  .affectedRows,
              2U);
    container.flushDirtyPages();
  }

  {
    Container reopened(databaseFile.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_FALSE(DatabaseStartup::execute(reopened));

    // Reading the persisted BOOLEAN catalog type and inserting again verifies
    // the RDB adapter reconstructs the canonical descriptor after restart.
    const auto inserted = reopened.executeQueryAction().execute(
        "INSERT INTO flags (enabled, visible) VALUES (TRUE, TRUE);");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            inserted));
    EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionResult>(inserted)
                  .affectedRows,
              1U);
    reopened.flushDirtyPages();
  }

  std::filesystem::remove(databaseFile);
}

TEST(ExecuteQueryActionIntegrationTest,
     InsertsAndSelectsRealAndDoublePrecisionValues) {
  const auto databaseFile =
      std::filesystem::temp_directory_path() / "unidb_floating_point_values_test.db";
  std::filesystem::remove(databaseFile);

  {
    Container container(databaseFile.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));

    const auto created = container.executeQueryAction().execute(
        "CREATE TABLE measurements (approximate REAL, precise DOUBLE PRECISION);");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            created));

    const auto inserted = container.executeQueryAction().execute(
        "INSERT INTO measurements (approximate, precise) VALUES "
        "(1.25, 1.234567890123), (-2.5, 1e3), (7, 8);");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            inserted))
        << std::get<QueryExecutionService::QueryExecutionError>(inserted).message;
    EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionResult>(inserted)
                  .affectedRows,
              3U);

    const auto selected =
        container.executeQueryAction().execute("SELECT * FROM measurements;");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            selected));
    const auto &result =
        std::get<QueryExecutionService::QueryExecutionResult>(selected);
    ASSERT_EQ(result.rows.size(), 3U);
    ASSERT_EQ(result.rows[0].size(), 2U);
    EXPECT_FLOAT_EQ(std::get<float>(result.rows[0][0]), 1.25F);
    EXPECT_DOUBLE_EQ(std::get<double>(result.rows[0][1]), 1.234567890123);
    EXPECT_FLOAT_EQ(std::get<float>(result.rows[1][0]), -2.5F);
    EXPECT_DOUBLE_EQ(std::get<double>(result.rows[1][1]), 1000.0);
    EXPECT_FLOAT_EQ(std::get<float>(result.rows[2][0]), 7.0F);
    EXPECT_DOUBLE_EQ(std::get<double>(result.rows[2][1]), 8.0);

    const auto overflow = container.executeQueryAction().execute(
        "INSERT INTO measurements (approximate, precise) VALUES "
        "(3.5e38, 9.0);");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionError>(
            overflow));
    const auto afterOverflow =
        container.executeQueryAction().execute("SELECT * FROM measurements;");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            afterOverflow));
    EXPECT_EQ(std::get<QueryExecutionService::QueryExecutionResult>(afterOverflow)
                  .rows.size(),
              3U);
    container.flushDirtyPages();
  }

  std::filesystem::remove(databaseFile);
}

TEST(ExecuteQueryActionIntegrationTest,
     PersistsFloatingPointColumnTypesAcrossContainerRestart) {
  const auto databaseFile = std::filesystem::temp_directory_path() /
                            "unidb_floating_point_schema_test.db";
  std::filesystem::remove(databaseFile);

  {
    Container container(databaseFile.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));

    const auto created = container.executeQueryAction().execute(
        "CREATE TABLE measurements (approximate REAL, precise DOUBLE "
        "PRECISION);");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            created));
    container.flushDirtyPages();
  }

  {
    Container reopened(databaseFile.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_FALSE(DatabaseStartup::execute(reopened));

    // A SELECT after restart forces the RDB adapter to reconstruct the
    // canonical floating-point descriptors from persisted catalog metadata.
    const auto selected =
        reopened.executeQueryAction().execute("SELECT * FROM measurements;");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            selected));
    const auto &result =
        std::get<QueryExecutionService::QueryExecutionResult>(selected);
    EXPECT_TRUE(result.rows.empty());
    ASSERT_EQ(result.columns.size(), 2U);
    ASSERT_TRUE(result.columns[0].declaredType.has_value());
    ASSERT_TRUE(result.columns[1].declaredType.has_value());
    EXPECT_EQ(result.columns[0].declaredType->id, unidb::sql::SqlTypeId::Real);
    EXPECT_EQ(result.columns[1].declaredType->id,
              unidb::sql::SqlTypeId::DoublePrecision);
  }

  std::filesystem::remove(databaseFile);
}
