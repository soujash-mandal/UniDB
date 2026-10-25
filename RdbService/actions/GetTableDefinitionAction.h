#pragma once

#include "../domain/RdbTable.h"
#include "../port/GetCatalogRootPageIdPort.h"
#include "../port/GetCatalogTableByNamePort.h"

#include <cstdint>
#include <string>

namespace RdbService {

class GetTableDefinitionAction {
public:
  GetTableDefinitionAction(
      GetCatalogRootPageIdPort &getCatalogRootPageIdPort,
      GetCatalogTableByNamePort &getCatalogTableByNamePort);

  RdbTableDefinition execute(uint32_t metadataPageId,
                             const std::string &tableName);

private:
  GetCatalogRootPageIdPort &getCatalogRootPageIdPort;
  GetCatalogTableByNamePort &getCatalogTableByNamePort;
};

} // namespace RdbService
