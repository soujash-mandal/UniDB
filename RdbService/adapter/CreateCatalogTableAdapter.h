#pragma once

#include "../../CatalogService/actions/CreateTableAction.h"
#include "../port/CreateCatalogTablePort.h"

class CreateCatalogTableAdapter final : public CreateCatalogTablePort {
public:
  explicit CreateCatalogTableAdapter(CreateTableAction &action);

  uint32_t createTable(uint32_t catalogRootPageId, uint32_t tableId,
                       const RdbTableDefinition &table) override;

private:
  CreateTableAction &action;
};
