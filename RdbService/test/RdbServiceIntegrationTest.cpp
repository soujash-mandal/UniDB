#include "../../DatabaseStartup.h"
#include "../../EvictionPolicy/EvictionPolicyType.h"
#include "../actions/CreateTableAction.h"
#include "../actions/InsertRowAction.h"
#include "../actions/SelectRowsAction.h"
#include "../actions/GetTableDefinitionAction.h"
#include "../actions/DeleteRowsAction.h"
#include "../domain/RdbResult.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <string>

namespace {
std::filesystem::path testDatabasePath(const std::string &name) {
  return std::filesystem::temp_directory_path() / ("unidb_rdb_service_" + name + ".db");
}
}

TEST(RdbServiceIntegrationTest, CreateInsertSelectAndPersistRows) {
  const auto path = testDatabasePath("persist");
  std::filesystem::remove(path);
  {
    Container container(path.string(), 8, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));
    auto created = container.rdbCreateTableAction().execute({"users", {{"id", unidb::sql::SqlType::Int}, {"name", unidb::sql::SqlType::Text}}});
    ASSERT_TRUE(created.hasValue()) << (created.hasValue() ? "" : created.error().message);
    auto inserted = container.insertRowAction().execute({"users", {"id", "name"}, {std::int64_t{7}, std::string{"Ada"}}});
    ASSERT_TRUE(inserted.hasValue()) << (inserted.hasValue() ? "" : inserted.error().message);
    EXPECT_EQ(inserted.value(), 1u);
    auto selected = container.selectRowsAction().execute({true, {}, "users", std::monostate{}});
    ASSERT_TRUE(selected.hasValue()) << (selected.hasValue() ? "" : selected.error().message);
    ASSERT_EQ(selected.value().size(), 1u);
    ASSERT_EQ(selected.value()[0].size(), 2u);
    EXPECT_EQ(std::get<std::int64_t>(selected.value()[0][0]), 7);
    EXPECT_EQ(std::get<std::string>(selected.value()[0][1]), "Ada");
  }
  {
    Container container(path.string(), 8, EvictionPolicyType::FIFO);
    ASSERT_FALSE(DatabaseStartup::execute(container));
    auto selected = container.selectRowsAction().execute({true, {}, "users", std::monostate{}});
    ASSERT_TRUE(selected.hasValue()) << (selected.hasValue() ? "" : selected.error().message);
    ASSERT_EQ(selected.value().size(), 1u);
    EXPECT_EQ(std::get<std::string>(selected.value()[0][1]), "Ada");
  }
  std::filesystem::remove(path);
}

TEST(RdbServiceIntegrationTest, SelectSupportsProjectionAndEqualityPredicate) {
  const auto path = testDatabasePath("projection");
  std::filesystem::remove(path);
  {
    Container container(path.string(), 8, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));
    ASSERT_TRUE(container.rdbCreateTableAction().execute({"items", {{"id", unidb::sql::SqlType::Int}, {"label", unidb::sql::SqlType::Text}}}).hasValue());
    ASSERT_TRUE(container.insertRowAction().execute({"items", {"id", "label"}, {std::int64_t{1}, std::string{"one"}}}).hasValue());
    ASSERT_TRUE(container.insertRowAction().execute({"items", {"id", "label"}, {std::int64_t{2}, std::string{"two"}}}).hasValue());
    auto selected = container.selectRowsAction().execute({false, {"label"}, "items", unidb::sql::WhereEquals{"id", std::int64_t{2}}});
    ASSERT_TRUE(selected.hasValue());
    ASSERT_EQ(selected.value().size(), 1u);
    ASSERT_EQ(selected.value()[0].size(), 1u);
    EXPECT_EQ(std::get<std::string>(selected.value()[0][0]), "two");
  }
  std::filesystem::remove(path);
}

TEST(RdbServiceIntegrationTest, GetsDefinitionAndDeletesOnlyMatchingRows) {
  const auto path = testDatabasePath("delete");
  std::filesystem::remove(path);
  {
    Container container(path.string(), 8, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));
    ASSERT_TRUE(container.rdbCreateTableAction().execute({"events", {{"id", unidb::sql::SqlType::Int}, {"label", unidb::sql::SqlType::Text}}}).hasValue());
    ASSERT_TRUE(container.insertRowAction().execute({"events", {"id", "label"}, {std::int64_t{1}, std::string{"keep"}}}).hasValue());
    ASSERT_TRUE(container.insertRowAction().execute({"events", {"id", "label"}, {std::int64_t{2}, std::string{"remove"}}}).hasValue());
    auto definition = container.getTableDefinitionAction().execute("events");
    ASSERT_TRUE(definition.hasValue());
    EXPECT_EQ(definition.value().tableName, "events");
    ASSERT_EQ(definition.value().columns.size(), 2u);
    EXPECT_EQ(definition.value().columns[0].name, "id");
    auto deleted = container.deleteRowsAction().execute("events", {"id", std::int64_t{2}});
    ASSERT_TRUE(deleted.hasValue());
    EXPECT_EQ(deleted.value(), 1u);
    auto selected = container.selectRowsAction().execute({true, {}, "events", std::monostate{}});
    ASSERT_TRUE(selected.hasValue());
    ASSERT_EQ(selected.value().size(), 1u);
    EXPECT_EQ(std::get<std::string>(selected.value()[0][1]), "keep");
    auto noMatch = container.deleteRowsAction().execute("events", {"id", std::int64_t{99}});
    ASSERT_TRUE(noMatch.hasValue());
    EXPECT_EQ(noMatch.value(), 0u);
  }
  std::filesystem::remove(path);
}

TEST(RdbServiceIntegrationTest, ReturnsStructuredErrorsForInvalidColumnsAndDuplicateTables) {
  const auto path = testDatabasePath("errors");
  std::filesystem::remove(path);
  {
    Container container(path.string(), 8, EvictionPolicyType::FIFO);
    ASSERT_TRUE(DatabaseStartup::execute(container));
    ASSERT_TRUE(container.rdbCreateTableAction().execute({"items", {{"id", unidb::sql::SqlType::Int}}}).hasValue());
    auto duplicate = container.rdbCreateTableAction().execute({"items", {{"id", unidb::sql::SqlType::Int}}});
    ASSERT_FALSE(duplicate.hasValue());
    EXPECT_EQ(duplicate.error().code, unidb::rdb::RdbErrorCode::DuplicateTable);
    auto badProjection = container.selectRowsAction().execute({false, {"missing"}, "items", std::monostate{}});
    ASSERT_FALSE(badProjection.hasValue());
    EXPECT_EQ(badProjection.error().code, unidb::rdb::RdbErrorCode::UnknownColumn);
    auto badPredicate = container.deleteRowsAction().execute("items", {"missing", std::int64_t{1}});
    ASSERT_FALSE(badPredicate.hasValue());
    EXPECT_EQ(badPredicate.error().code, unidb::rdb::RdbErrorCode::UnknownColumn);
    auto missingTable = container.getTableDefinitionAction().execute("missing");
    ASSERT_FALSE(missingTable.hasValue());
    EXPECT_EQ(missingTable.error().code, unidb::rdb::RdbErrorCode::TableNotFound);
  }
  std::filesystem::remove(path);
}
