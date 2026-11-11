#include "../../../DatabaseStartup.h"
#include "../../../container/container.h"
#include "../../domain/parser/SqlParserImpl.h"

#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <variant>

using unidb::sql::SqlTypeId;

TEST(ExecuteQueryActionIntegrationTest,
     PersistsPostgreSqlDateTimeColumnDescriptorsAcrossRestart) {
  const auto databaseFile = std::filesystem::temp_directory_path() /
                            "unidb_datetime_schema_integration_test.db";
  std::filesystem::remove(databaseFile);

  {
    Container container(databaseFile.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));

    const auto created = container.executeQueryAction().execute(
        "CREATE TABLE temporal_values (calendar_date DATE, "
        "local_time TIME(3), offset_time TIMETZ(4), "
        "local_timestamp TIMESTAMP(5), instant TIMESTAMPTZ(6), "
        "elapsed INTERVAL);");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            created))
        << std::get<QueryExecutionService::QueryExecutionError>(created)
               .message;

    container.flushDirtyPages();
  }

  {
    Container reopened(databaseFile.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_FALSE(DatabaseStartup::execute(reopened));

    const auto selected =
        reopened.executeQueryAction().execute("SELECT * FROM temporal_values;");
    ASSERT_TRUE(
        std::holds_alternative<QueryExecutionService::QueryExecutionResult>(
            selected))
        << std::get<QueryExecutionService::QueryExecutionError>(selected)
               .message;

    const auto &result =
        std::get<QueryExecutionService::QueryExecutionResult>(selected);
    EXPECT_TRUE(result.rows.empty());
    ASSERT_EQ(result.columns.size(), 6U);

    const auto &columns = result.columns;
    ASSERT_TRUE(columns[0].declaredType.has_value());
    EXPECT_EQ(columns[0].declaredType->id, SqlTypeId::Date);
    EXPECT_FALSE(
        columns[0].declaredType->modifier.temporalPrecision.has_value());

    ASSERT_TRUE(columns[1].declaredType.has_value());
    EXPECT_EQ(columns[1].declaredType->id, SqlTypeId::Time);
    EXPECT_EQ(columns[1].declaredType->modifier.temporalPrecision, 3U);

    ASSERT_TRUE(columns[2].declaredType.has_value());
    EXPECT_EQ(columns[2].declaredType->id, SqlTypeId::TimeWithTimeZone);
    EXPECT_EQ(columns[2].declaredType->modifier.temporalPrecision, 4U);

    ASSERT_TRUE(columns[3].declaredType.has_value());
    EXPECT_EQ(columns[3].declaredType->id, SqlTypeId::Timestamp);
    EXPECT_EQ(columns[3].declaredType->modifier.temporalPrecision, 5U);

    ASSERT_TRUE(columns[4].declaredType.has_value());
    EXPECT_EQ(columns[4].declaredType->id, SqlTypeId::TimestampWithTimeZone);
    EXPECT_EQ(columns[4].declaredType->modifier.temporalPrecision, 6U);

    ASSERT_TRUE(columns[5].declaredType.has_value());
    EXPECT_EQ(columns[5].declaredType->id, SqlTypeId::Interval);
    EXPECT_FALSE(
        columns[5].declaredType->modifier.temporalPrecision.has_value());
  }

  std::filesystem::remove(databaseFile);
}

TEST(SqlDateTimeDeclarationTest, RejectsPrecisionForDateAndInterval) {
  unidb::sql::SqlParserImpl parser;
  const std::array<const char *, 2> invalidDeclarations{
      "CREATE TABLE invalid_date (value DATE(2));",
      "CREATE TABLE invalid_interval (value INTERVAL(2));"};

  for (const auto *sql : invalidDeclarations) {
    const auto result = parser.parse(sql);
    ASSERT_TRUE(std::holds_alternative<unidb::sql::SqlParseError>(result))
        << sql;
  }
}
