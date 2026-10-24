#include "GetTableDefinitionAction.h"

#include <stdexcept>

GetTableDefinitionAction::GetTableDefinitionAction(
    RdbMetadataPort &metadataPort, CatalogTablePort &catalogTablePort)
    : metadataPort(metadataPort), catalogTablePort(catalogTablePort) {}

RdbTableDefinition
GetTableDefinitionAction::execute(const std::string &tableName) {
  if (tableName.empty()) {
    throw std::invalid_argument("Table name cannot be empty");
  }
  const CatalogPageId catalogRootPageId =
      metadataPort.getCatalogRootPageId();
  return catalogTablePort.getTableByName(catalogRootPageId, tableName);
}
