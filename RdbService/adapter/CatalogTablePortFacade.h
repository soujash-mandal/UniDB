#pragma once

#include "../port/CatalogTablePort.h"
#include "CreateCatalogTableAdapter.h"
#include "GetCatalogTableByNameAdapter.h"
#include "UpdateCatalogTableAdapter.h"

class CatalogTablePortFacade final : public CatalogTablePort {
public:
  CatalogTablePortFacade(CreateCatalogTableAdapter &createAdapter,
                         GetCatalogTableByNameAdapter &getAdapter,
                         UpdateCatalogTableAdapter &updateAdapter);

  uint32_t createTable(uint32_t catalogRootPageId, uint32_t tableId,
                       const RdbTableDefinition &table) override;
  RdbTableDefinition getTableByName(uint32_t catalogRootPageId,
                                    const std::string &tableName) override;
  void updateTable(uint32_t catalogRootPageId,
                   const RdbTableDefinition &table) override;

private:
  CreateCatalogTableAdapter &createAdapter;
  GetCatalogTableByNameAdapter &getAdapter;
  UpdateCatalogTableAdapter &updateAdapter;
};
