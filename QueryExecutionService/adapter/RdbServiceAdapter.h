#pragma once
#include "../port/RdbServicePort.h"
#include "../../RdbService/actions/CreateTableAction.h"
#include "../../RdbService/actions/DropTableAction.h"
#include "../../RdbService/actions/GetTableDefinitionAction.h"
#include "../../RdbService/actions/InsertRowAction.h"
#include "../../RdbService/actions/SelectRowsAction.h"
#include "../../RdbService/actions/DeleteRowsAction.h"
namespace unidb::query {
class RdbServiceAdapter final : public RdbServicePort {
public:
  RdbServiceAdapter(unidb::rdb::CreateTableAction &, unidb::rdb::DropTableAction &, unidb::rdb::GetTableDefinitionAction &, unidb::rdb::InsertRowAction &, unidb::rdb::SelectRowsAction &, unidb::rdb::DeleteRowsAction &);
  unidb::rdb::RdbResult<uint32_t> createTable(const unidb::sql::CreateTableStatement &) override;
  unidb::rdb::RdbStatus dropTable(const std::string &) override;
  unidb::rdb::RdbResult<unidb::rdb::TableDefinition> getTableDefinition(const std::string &) override;
  unidb::rdb::RdbResult<std::size_t> insertRow(const unidb::sql::InsertStatement &) override;
  unidb::rdb::RdbResult<std::vector<std::vector<unidb::sql::SqlValue>>> selectRows(const unidb::sql::SelectStatement &) override;
  unidb::rdb::RdbResult<std::size_t> deleteRows(const std::string &, const unidb::sql::WhereEquals &) override;
private:
  unidb::rdb::CreateTableAction &create;
  unidb::rdb::DropTableAction &drop;
  unidb::rdb::GetTableDefinitionAction &getDefinition;
  unidb::rdb::InsertRowAction &insert;
  unidb::rdb::SelectRowsAction &select;
  unidb::rdb::DeleteRowsAction &erase;
};
}