#pragma once

#include "../../CatalogService/actions/CreateTableAction.h"
#include "../domain/RdbTable.h"

class CreateCatalogTableAdapter final {
public:
  explicit CreateCatalogTableAdapter(CreateTableAction &action);
  uint32_t createTable(uint32_t catalogRootPageId, uint32_t tableId,
                       const RdbTableDefinition &table);

private:
  CreateTableAction &action;
};
