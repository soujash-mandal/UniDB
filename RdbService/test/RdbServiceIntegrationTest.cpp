#include "../../DatabaseStartup.h"
#include "../../EvictionPolicy/EvictionPolicyType.h"
#include "../actions/CreateTableAction.h"
#include "../actions/DeleteRowsAction.h"
#include "../actions/GetTableDefinitionAction.h"
#include "../actions/InsertRowAction.h"
#include "../actions/SelectRowsAction.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <stdexcept>

namespace {
std::filesystem::path testDatabasePath() {
  return std::filesystem::temp_directory_path() /
         "unidb_rdb_service_integration_test.db";
}
} // namespace

TEST(RdbServiceIntegrationTest, CreateInsertSelectAndPersistRows) {
  const auto path = testDatabasePath();
  std::filesystem::remove(path);
  {
    Container container(path.string(), 8, EvictionPolicyType::FIFO);
    EXPECT_TRUE(DatabaseStartup::execute(container));
    unidb::sql::CreateTableStatement create{
        "users",
        {{"id", unidb::sql::SqlType::Int},
         {"name", unidb::sql::SqlType::Text}}};
    EXPECT_NO_THROW(container.rdbCreateTableAction().execute(create));
    unidb::sql::InsertStatement insert{
        "users", {}, {std::int64_t{7}, std::string{"Ada"}}};
    EXPECT_NO_THROW(container.insertRowAction().execute(insert));
    unidb::sql::SelectStatement select{true, {}, "users", std::monostate{}};
    const auto rows = container.selectRowsAction().execute(select);
    ASSERT_EQ(rows.size(), 1u);
    ASSERT_EQ(rows[0].size(), 2u);
    EXPECT_EQ(std::get<std::int64_t>(rows[0][0]), 7);
    EXPECT_EQ(std::get<std::string>(rows[0][1]), "Ada");
    container.close();
  }
  {
    Container container(path.string(), 8, EvictionPolicyType::FIFO);
    EXPECT_FALSE(DatabaseStartup::execute(container));
    unidb::sql::SelectStatement select{true, {}, "users", std::monostate{}};
    const auto rows = container.selectRowsAction().execute(select);
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(std::get<std::string>(rows[0][1]), "Ada");
  }
  std::filesystem::remove(path);
}

TEST(RdbServiceIntegrationTest, SelectSupportsProjectionAndEqualityPredicate) {
  const auto path = testDatabasePath();
  std::filesystem::remove(path);
  {
    Container container(path.string(), 8, EvictionPolicyType::FIFO);
    DatabaseStartup::execute(container);
    unidb::sql::CreateTableStatement create{
        "items",
        {{"id", unidb::sql::SqlType::Int},
         {"label", unidb::sql::SqlType::Text}}};
    container.rdbCreateTableAction().execute(create);
    container.insertRowAction().execute(
        {"items", {}, {std::int64_t{1}, std::string{"one"}}});
    container.insertRowAction().execute(
        {"items", {}, {std::int64_t{2}, std::string{"two"}}});
    unidb::sql::SelectStatement select{
        false,
        {"label"},
        "items",
        unidb::sql::WhereEquals{"id", std::int64_t{2}}};
    const auto rows = container.selectRowsAction().execute(select);
    ASSERT_EQ(rows.size(), 1u);
    ASSERT_EQ(rows[0].size(), 1u);
    EXPECT_EQ(std::get<std::string>(rows[0][0]), "two");
  }
  std::filesystem::remove(path);
}

TEST(RdbServiceIntegrationTest, GetDefinitionAndDeleteMatchingRows) {
  const auto path = testDatabasePath();
  std::filesystem::remove(path);
  {
    Container container(path.string(), 8, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));
    container.rdbCreateTableAction().execute(
        {"events",
         {{"id", unidb::sql::SqlType::Int},
          {"label", unidb::sql::SqlType::Text}}});
    container.insertRowAction().execute(
        {"events", {}, {std::int64_t{1}, std::string{"keep"}}});
    container.insertRowAction().execute(
        {"events", {}, {std::int64_t{2}, std::string{"remove"}}});
    const auto definition =
        container.getTableDefinitionAction().execute("events");
    EXPECT_EQ(definition.tableName, "events");
    ASSERT_EQ(definition.columns.size(), 2u);
    EXPECT_EQ(definition.columns[0].name, "id");
    const auto deleted =
        container.deleteRowsAction().execute("events", {"id", std::int64_t{2}});
    EXPECT_EQ(deleted, 1u);
    const auto rows = container.selectRowsAction().execute(
        {true, {}, "events", std::monostate{}});
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(std::get<std::string>(rows[0][1]), "keep");
    EXPECT_EQ(container.deleteRowsAction().execute("events",
                                                   {"id", std::int64_t{99}}),
              0u);
  }
  std::filesystem::remove(path);
}

TEST(RdbServiceIntegrationTest, RejectsUnknownProjectionAndPredicateColumns) {
  const auto path = testDatabasePath();
  std::filesystem::remove(path);
  {
    Container container(path.string(), 8, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));
    container.rdbCreateTableAction().execute(
        {"items", {{"id", unidb::sql::SqlType::Int}}});
    EXPECT_THROW(container.selectRowsAction().execute(
                     {false, {"missing"}, "items", std::monostate{}}),
                 std::invalid_argument);
    EXPECT_THROW(container.deleteRowsAction().execute(
                     "items", {"missing", std::int64_t{1}}),
                 std::invalid_argument);
    EXPECT_THROW(container.selectRowsAction().execute(
                     {true,
                      {},
                      "items",
                      unidb::sql::WhereEquals{"id", std::string{"wrong"}}}),
                 std::invalid_argument);
    EXPECT_THROW(container.deleteRowsAction().execute(
                     "items", {"id", std::string{"wrong"}}),
                 std::invalid_argument);
  }
  std::filesystem::remove(path);
}

TEST(RdbServiceIntegrationTest,
     DroppingPopulatedTableReleasesPagesWithoutStaleRows) {
  const auto path = testDatabasePath();
  std::filesystem::remove(path);
  {
    Container container(path.string(), 8, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));
    container.rdbCreateTableAction().execute(
        {"old_rows",
         {{"id", unidb::sql::SqlType::Int},
          {"payload", unidb::sql::SqlType::Text}}});
    const std::string payload(1200, 'x');
    for (int64_t id = 0; id < 20; ++id) {
      container.insertRowAction().execute({"old_rows", {}, {id, payload}});
    }

    container.rdbDropTableAction().execute("old_rows");
    EXPECT_THROW(container.getTableDefinitionAction().execute("old_rows"),
                 std::runtime_error);

    container.rdbCreateTableAction().execute(
        {"new_rows",
         {{"id", unidb::sql::SqlType::Int},
          {"payload", unidb::sql::SqlType::Text}}});
    container.insertRowAction().execute(
        {"new_rows", {}, {std::int64_t{99}, std::string{"fresh"}}});
    const auto rows = container.selectRowsAction().execute(
        {true, {}, "new_rows", std::monostate{}});
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(std::get<std::int64_t>(rows[0][0]), 99);
    EXPECT_EQ(std::get<std::string>(rows[0][1]), "fresh");
    container.close();
  }
  std::filesystem::remove(path);
}
