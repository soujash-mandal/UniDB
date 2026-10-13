#include "../../DatabaseStartup.h"
#include "../../EvictionPolicy/EvictionPolicyType.h"
#include "../actions/CreateTableAction.h"
#include "../actions/InsertRowAction.h"
#include "../actions/SelectRowsAction.h"
#include <gtest/gtest.h>
#include <filesystem>

namespace {
std::filesystem::path testDatabasePath() {
  return std::filesystem::temp_directory_path() / "unidb_rdb_service_integration_test.db";
}
}

TEST(RdbServiceIntegrationTest, CreateInsertSelectAndPersistRows) {
  const auto path = testDatabasePath();
  std::filesystem::remove(path);
  {
    Container container(path.string(), 8, EvictionPolicyType::FIFO);
    EXPECT_TRUE(DatabaseStartup::execute(container));
    unidb::sql::CreateTableStatement create{"users", {{"id", unidb::sql::SqlType::Int}, {"name", unidb::sql::SqlType::Text}}};
    EXPECT_NO_THROW(container.rdbCreateTableAction().execute(create));
    unidb::sql::InsertStatement insert{"users", {}, {std::int64_t{7}, std::string{"Ada"}}};
    EXPECT_NO_THROW(container.insertRowAction().execute(insert));
    unidb::sql::SelectStatement select{true, {}, "users", std::monostate{}};
    const auto rows = container.selectRowsAction().execute(select);
    ASSERT_EQ(rows.size(), 1u);
    ASSERT_EQ(rows[0].size(), 2u);
    EXPECT_EQ(std::get<std::int64_t>(rows[0][0]), 7);
    EXPECT_EQ(std::get<std::string>(rows[0][1]), "Ada");
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
    unidb::sql::CreateTableStatement create{"items", {{"id", unidb::sql::SqlType::Int}, {"label", unidb::sql::SqlType::Text}}};
    container.rdbCreateTableAction().execute(create);
    container.insertRowAction().execute({"items", {}, {std::int64_t{1}, std::string{"one"}}});
    container.insertRowAction().execute({"items", {}, {std::int64_t{2}, std::string{"two"}}});
    unidb::sql::SelectStatement select{false, {"label"}, "items", unidb::sql::WhereEquals{"id", std::int64_t{2}}};
    const auto rows = container.selectRowsAction().execute(select);
    ASSERT_EQ(rows.size(), 1u);
    ASSERT_EQ(rows[0].size(), 1u);
    EXPECT_EQ(std::get<std::string>(rows[0][0]), "two");
  }
  std::filesystem::remove(path);
}
