#pragma once
#include "../domain/RdbTable.h"
#include "../port/GetCatalogRootPageIdPort.h"
#include "../port/GetCatalogTablesPort.h"
#include <cstdint>
#include <vector>
namespace RdbService {
class GetAllTableDefinitionsAction {
public:
  GetAllTableDefinitionsAction(GetCatalogRootPageIdPort &, GetCatalogTablesPort &);
  std::vector<RdbTableDefinition> execute(uint32_t metadataPageId);
private:
  GetCatalogRootPageIdPort &getCatalogRootPageIdPort;
  GetCatalogTablesPort &getCatalogTablesPort;
};
}
