#pragma once

#include "../../CatalogService/actions/UpdateTableAction.h"
#include "../domain/RdbTable.h"

class UpdateCatalogTableAdapter final {
public:
  explicit UpdateCatalogTableAdapter(UpdateTableAction &action);

  void updateTable(uint32_t catalogRootPageId, const RdbTableDefinition &table,
                   uint32_t firstSpaceMapPageId);

private:
  UpdateTableAction &action;
};
