#pragma once

#include "../../CatalogService/actions/CreateTableAction.h"
#include "../../CatalogService/actions/GetTableByNameAction.h"
#include "../../CatalogService/actions/UpdateTableAction.h"
#include "../port/CatalogTablePort.h"

class CatalogTableAdapter final : public CatalogTablePort {
public:
  CatalogTableAdapter(CreateTableAction &createTableAction,
                     GetTableByNameAction &getTableByNameAction,
                     UpdateTableAction &updateTableAction);

  uint32_t createTable(uint32_t catalogRootPageId, uint32_t tableId,
                       const RdbTableDefinition &table) override;
  RdbTableDefinition getTableByName(uint32_t catalogRootPageId,
                                    const std::string &tableName) override;
  void updateTable(uint32_t catalogRootPageId,
                   const RdbTableDefinition &table) override;

private:
  CreateTableAction &createTableAction;
  GetTableByNameAction &getTableByNameAction;
  UpdateTableAction &updateTableAction;
};
