#include "CatalogTablePortFacade.h"

CatalogTablePortFacade::CatalogTablePortFacade(
    CreateCatalogTableAdapter &createAdapter,
    GetCatalogTableByNameAdapter &getAdapter,
    UpdateCatalogTableAdapter &updateAdapter)
    : createAdapter(createAdapter), getAdapter(getAdapter),
      updateAdapter(updateAdapter) {}

uint32_t CatalogTablePortFacade::createTable(
    uint32_t catalogRootPageId, uint32_t tableId,
    const RdbTableDefinition &table) {
  return createAdapter.createTable(catalogRootPageId, tableId, table);
}

RdbTableDefinition CatalogTablePortFacade::getTableByName(
    uint32_t catalogRootPageId, const std::string &tableName) {
  return getAdapter.getTableByName(catalogRootPageId, tableName);
}

void CatalogTablePortFacade::updateTable(
    uint32_t catalogRootPageId, const RdbTableDefinition &table) {
  updateAdapter.updateTable(catalogRootPageId, table);
}
