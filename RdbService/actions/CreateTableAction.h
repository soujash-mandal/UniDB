#pragma once

#include "../domain/RdbTable.h"
#include "../port/AllocateNextTableIdPort.h"
#include "../port/CreateCatalogTablePort.h"
#include "../port/GetCatalogRootPageIdPort.h"

#include <cstdint>

namespace RdbService {

class CreateTableAction {
public:
  CreateTableAction(GetCatalogRootPageIdPort &getCatalogRootPageIdPort,
                    AllocateNextTableIdPort &allocateNextTableIdPort,
                    CreateCatalogTablePort &createCatalogTablePort);

  uint32_t execute(uint32_t metadataPageId, RdbTableDefinition table);

private:
  GetCatalogRootPageIdPort &getCatalogRootPageIdPort;
  AllocateNextTableIdPort &allocateNextTableIdPort;
  CreateCatalogTablePort &createCatalogTablePort;
};

} // namespace RdbService
