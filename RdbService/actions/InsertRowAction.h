#pragma once

#include "../domain/RdbTable.h"
#include "../port/CreateSpaceMapPagePort.h"
#include "../port/CreateTuplePort.h"
#include "../port/FindPageWithSpacePort.h"
#include "../port/GetCatalogRootPageIdPort.h"
#include "../port/GetCatalogTableByNamePort.h"
#include "../port/UpdateCatalogTablePort.h"

#include <cstdint>
#include <string>
#include <vector>

namespace RdbService {

struct InsertedRowLocation {
  uint32_t tuplePageId;
  uint32_t slotId;
};

class InsertRowAction {
public:
  InsertRowAction(GetCatalogRootPageIdPort &getCatalogRootPageIdPort,
                  GetCatalogTableByNamePort &getCatalogTableByNamePort,
                  UpdateCatalogTablePort &updateCatalogTablePort,
                  CreateSpaceMapPagePort &createSpaceMapPagePort,
                  FindPageWithSpacePort &findPageWithSpacePort,
                  CreateTuplePort &createTuplePort);

  InsertedRowLocation execute(uint32_t metadataPageId,
                              const std::string &tableName,
                              std::vector<char> tupleData);

private:
  GetCatalogRootPageIdPort &getCatalogRootPageIdPort;
  GetCatalogTableByNamePort &getCatalogTableByNamePort;
  UpdateCatalogTablePort &updateCatalogTablePort;
  CreateSpaceMapPagePort &createSpaceMapPagePort;
  FindPageWithSpacePort &findPageWithSpacePort;
  CreateTuplePort &createTuplePort;
};

} // namespace RdbService
