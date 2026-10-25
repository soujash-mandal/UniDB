#include "GetTableDefinitionAction.h"

#include <stdexcept>

namespace RdbService {

GetTableDefinitionAction::GetTableDefinitionAction(
    GetCatalogRootPageIdPort &getCatalogRootPageIdPort,
    GetCatalogTableByNamePort &getCatalogTableByNamePort)
    : getCatalogRootPageIdPort(getCatalogRootPageIdPort),
      getCatalogTableByNamePort(getCatalogTableByNamePort) {}

RdbTableDefinition
GetTableDefinitionAction::execute(uint32_t metadataPageId,
                                  const std::string &tableName) {
  if (tableName.empty()) {
    throw std::invalid_argument("Table name must not be empty");
  }

  const uint32_t catalogRootPageId =
      getCatalogRootPageIdPort.getCatalogRootPageId(metadataPageId);
  return getCatalogTableByNamePort.getTableByName(catalogRootPageId,
                                                  tableName);
}

} // namespace RdbService
