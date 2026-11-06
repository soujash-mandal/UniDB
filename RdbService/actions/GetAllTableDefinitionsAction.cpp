#include "GetAllTableDefinitionsAction.h"
namespace RdbService {
GetAllTableDefinitionsAction::GetAllTableDefinitionsAction(
    GetCatalogRootPageIdPort &root, GetCatalogTablesPort &tables)
    : getCatalogRootPageIdPort(root), getCatalogTablesPort(tables) {}
std::vector<RdbTableDefinition>
GetAllTableDefinitionsAction::execute(uint32_t metadataPageId) {
  const auto root = getCatalogRootPageIdPort.getCatalogRootPageId(metadataPageId);
  return getCatalogTablesPort.getTables(root);
}
}
