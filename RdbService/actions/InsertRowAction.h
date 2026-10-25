#pragma once

#include "../domain/RdbTable.h"
#include "../port/CalculateRequiredSpacePort.h"
#include "../port/CreateSpaceMapPagePort.h"
#include "../port/CreateTupleWithFreeSpacePort.h"
#include "../port/FindPageWithSpacePort.h"
#include "../port/GetCatalogRootPageIdPort.h"
#include "../port/GetCatalogTableByNamePort.h"
#include "../port/UpdateCatalogTablePort.h"
#include "../port/UpdateFreeSpacePort.h"

#include <cstdint>
#include <string>
#include <vector>

namespace RdbService {

struct InsertedRowLocation {
  uint32_t tuplePageId;
  uint32_t slotId;
  uint32_t spaceMapPageId;
  uint32_t remainingFreeSpace;
  bool spaceMapUpdated;
  std::string spaceMapUpdateError;
};

class InsertRowAction {
public:
  InsertRowAction(GetCatalogRootPageIdPort &getCatalogRootPageIdPort,
                  GetCatalogTableByNamePort &getCatalogTableByNamePort,
                  UpdateCatalogTablePort &updateCatalogTablePort,
                  CreateSpaceMapPagePort &createSpaceMapPagePort,
                  CalculateRequiredSpacePort &calculateRequiredSpacePort,
                  FindPageWithSpacePort &findPageWithSpacePort,
                  CreateTupleWithFreeSpacePort &createTupleWithFreeSpacePort,
                  UpdateFreeSpacePort &updateFreeSpacePort);

  InsertedRowLocation execute(uint32_t metadataPageId,
                              const std::string &tableName,
                              std::vector<char> tupleData);

private:
  GetCatalogRootPageIdPort &getCatalogRootPageIdPort;
  GetCatalogTableByNamePort &getCatalogTableByNamePort;
  UpdateCatalogTablePort &updateCatalogTablePort;
  CreateSpaceMapPagePort &createSpaceMapPagePort;
  CalculateRequiredSpacePort &calculateRequiredSpacePort;
  FindPageWithSpacePort &findPageWithSpacePort;
  CreateTupleWithFreeSpacePort &createTupleWithFreeSpacePort;
  UpdateFreeSpacePort &updateFreeSpacePort;
};

} // namespace RdbService
