#include "../../../DatabaseStartup.h"
#include "../../../container/container.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <variant>

TEST(ExecuteQueryActionCharacterIntegrationTest,
     PersistsCharacterTypesAndNormalizedValuesAcrossRestart) {
  using Result = QueryExecutionService::QueryExecutionResult;
  const auto databaseFile = std::filesystem::temp_directory_path() /
                            "unidb_character_types_restart_test.db";
  std::filesystem::remove(databaseFile);

  {
    Container container(databaseFile.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));

    const auto created = container.executeQueryAction().execute(
        "CREATE TABLE labels (fixed CHAR(5), bounded VARCHAR(8), "
        "unbounded VARCHAR, description TEXT)");
    ASSERT_TRUE(std::holds_alternative<Result>(created))
        << std::get<QueryExecutionService::QueryExecutionError>(created)
               .message;

    const auto inserted = container.executeQueryAction().execute(
        "INSERT INTO labels (fixed, bounded, unbounded, description) "
        "VALUES ('cat', 'éclair', '  keep  ', 'hello 🌍')");
    ASSERT_TRUE(std::holds_alternative<Result>(inserted))
        << std::get<QueryExecutionService::QueryExecutionError>(inserted)
               .message;
    container.flushDirtyPages();
  }

  {
    Container reopened(databaseFile.string(), 4, EvictionPolicyType::FIFO);
    ASSERT_FALSE(DatabaseStartup::execute(reopened));

    const auto selected =
        reopened.executeQueryAction().execute("SELECT * FROM labels");
    ASSERT_TRUE(std::holds_alternative<Result>(selected))
        << std::get<QueryExecutionService::QueryExecutionError>(selected)
               .message;
    const auto &result =
        std::get<QueryExecutionService::QueryExecutionResult>(selected);
    ASSERT_EQ(result.rows.size(), 1U);
    ASSERT_EQ(result.rows[0].size(), 4U);
    EXPECT_EQ(std::get<std::string>(result.rows[0][0]), "cat  ");
    EXPECT_EQ(std::get<std::string>(result.rows[0][1]), "éclair");
    EXPECT_EQ(std::get<std::string>(result.rows[0][2]), "  keep  ");
    EXPECT_EQ(std::get<std::string>(result.rows[0][3]), "hello 🌍");

    ASSERT_TRUE(result.columns[0].declaredType.has_value());
    ASSERT_TRUE(result.columns[1].declaredType.has_value());
    ASSERT_TRUE(result.columns[2].declaredType.has_value());
    ASSERT_TRUE(result.columns[3].declaredType.has_value());
    EXPECT_EQ(result.columns[0].declaredType->id, unidb::sql::SqlTypeId::Char);
    EXPECT_EQ(result.columns[0].declaredType->modifier.length, 5U);
    EXPECT_EQ(result.columns[1].declaredType->id,
              unidb::sql::SqlTypeId::Varchar);
    EXPECT_EQ(result.columns[1].declaredType->modifier.length, 8U);
    EXPECT_EQ(result.columns[2].declaredType->id,
              unidb::sql::SqlTypeId::Varchar);
    EXPECT_FALSE(result.columns[2].declaredType->modifier.length.has_value());
    EXPECT_EQ(result.columns[3].declaredType->id, unidb::sql::SqlTypeId::Text);
  }

  std::filesystem::remove(databaseFile);
}
