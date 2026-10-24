#pragma once

#include "../port/UpdateCatalogTablePort.h"
#include "../../CatalogService/actions/UpdateTableAction.h"

class UpdateCatalogTableAdapter final : public UpdateCatalogTablePort {
public:
  explicit UpdateCatalogTableAdapter(UpdateTableAction &action);

  void updateTable(uint32_t catalogRootPageId, const RdbTableDefinition &table,
                   uint32_t firstSpaceMapPageId) override;

private:
  UpdateTableAction &action;
};
