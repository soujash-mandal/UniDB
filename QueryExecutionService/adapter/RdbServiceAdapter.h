#pragma once

#include "../port/RdbServicePort.h"
#include "../../RdbService/actions/CreateTableAction.h"
#include "../../RdbService/actions/GetTableDefinitionAction.h"
#include "../../RdbService/actions/InsertRowAction.h"
#include "../../RdbService/actions/SelectAllRowsAction.h"

namespace QueryExecutionService {

class RdbServiceAdapter final : public RdbServicePort {
public:
  RdbServiceAdapter(uint32_t metadataPageId,
                    RdbService::CreateTableAction &createTableAction,
                    RdbService::GetTableDefinitionAction &getTableDefinitionAction,
                    RdbService::InsertRowAction &insertRowAction,
                    RdbService::SelectAllRowsAction &selectAllRowsAction);

  uint32_t
  createTable(const unidb::sql::CreateTableStatement &statement) override;
  std::vector<unidb::sql::ColumnDefinition>
  getTableDefinition(const std::string &tableName) override;
  void insertRow(const std::string &tableName,
                 const std::vector<char> &tupleBytes) override;
  std::vector<std::vector<char>>
  selectAllRows(const std::string &tableName) override;

private:
  uint32_t metadataPageId;
  RdbService::CreateTableAction &createTableAction;
  RdbService::GetTableDefinitionAction &getTableDefinitionAction;
  RdbService::InsertRowAction &insertRowAction;
  RdbService::SelectAllRowsAction &selectAllRowsAction;
};

} // namespace QueryExecutionService
