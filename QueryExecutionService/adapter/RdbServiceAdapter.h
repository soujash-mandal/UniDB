#pragma once

#include "../port/RdbServicePort.h"
#include "../../RdbService/actions/CreateTableAction.h"
#include "../../RdbService/actions/DeleteRowsAction.h"
#include "../../RdbService/actions/DropTableAction.h"
#include "../../RdbService/actions/GetTableDefinitionAction.h"
#include "../../RdbService/actions/InsertRowAction.h"
#include "../../RdbService/actions/SelectRowsAction.h"

namespace unidb::query {
class RdbServiceAdapter final : public RdbServicePort {
public:
  RdbServiceAdapter(unidb::rdb::CreateTableAction &,
                    unidb::rdb::DropTableAction &,
                    unidb::rdb::GetTableDefinitionAction &,
                    unidb::rdb::InsertRowAction &,
                    unidb::rdb::SelectRowsAction &,
                    unidb::rdb::DeleteRowsAction &);

  RdbServiceResult<uint32_t>
  createTable(const unidb::sql::CreateTableStatement &) override;
  RdbServiceStatus dropTable(const std::string &) override;
  RdbServiceResult<QueryTableDefinition>
  getTableDefinition(const std::string &) override;
  RdbServiceResult<std::size_t>
  insertRow(const unidb::sql::InsertStatement &) override;
  RdbServiceResult<std::vector<std::vector<unidb::sql::SqlValue>>>
  selectRows(const unidb::sql::SelectStatement &) override;
  RdbServiceResult<std::size_t>
  deleteRows(const std::string &, const unidb::sql::WhereEquals &) override;

private:
  unidb::rdb::CreateTableAction &create;
  unidb::rdb::DropTableAction &drop;
  unidb::rdb::GetTableDefinitionAction &getDefinition;
  unidb::rdb::InsertRowAction &insert;
  unidb::rdb::SelectRowsAction &select;
  unidb::rdb::DeleteRowsAction &erase;
};
} // namespace unidb::query
