#pragma once

#include "../port/CreateCatalogTablePort.h"
#include "../../CatalogService/actions/CreateTableAction.h"

class CreateCatalogTableAdapter final : public CreateCatalogTablePort {
public:
  explicit CreateCatalogTableAdapter(CreateTableAction &action);

  uint32_t createTable(uint32_t catalogRootPageId, uint32_t tableId,
                       const RdbTableDefinition &table) override;

private:
  CreateTableAction &action;
};
