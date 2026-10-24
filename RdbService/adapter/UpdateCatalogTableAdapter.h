#pragma once

#include "../../CatalogService/actions/UpdateTableAction.h"
#include "../port/UpdateCatalogTablePort.h"

class UpdateCatalogTableAdapter final : public UpdateCatalogTablePort {
public:
  explicit UpdateCatalogTableAdapter(UpdateTableAction &action);

  void updateTable(uint32_t catalogRootPageId, const RdbTableDefinition &table,
                   uint32_t firstSpaceMapPageId) override;

private:
  UpdateTableAction &action;
};
