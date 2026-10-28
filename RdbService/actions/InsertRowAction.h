#pragma once

#include "../domain/RdbTable.h"
#include "../port/CalculateRequiredSpacePort.h"
#include "../port/CreateSpaceMapPagePort.h"
#include "../port/CreateTuplePort.h"
#include "../port/FindPageWithSpacePort.h"
#include "../port/GetCatalogRootPageIdPort.h"
#include "../port/GetCatalogTableByNamePort.h"
#include "../port/UpdateCatalogTablePort.h"
#include "../port/UpdateFreeSpacePort.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace RdbService {

struct InsertedRowLocation {
  uint32_t tuplePageId;
  uint32_t slotId;
};

// Signals that the row was committed, but its SpaceMap free-space entry could
// not be refreshed. Callers can inspect the location without retrying INSERT.
class SpaceMapUpdateFailedAfterInsert : public std::runtime_error {
public:
  SpaceMapUpdateFailedAfterInsert(InsertedRowLocation insertedRowLocation,
                                  const std::string &message)
      : std::runtime_error(message), insertedRowLocation(insertedRowLocation) {}

  const InsertedRowLocation &getInsertedRowLocation() const noexcept {
    return insertedRowLocation;
  }

private:
  InsertedRowLocation insertedRowLocation;
};

class InsertRowAction {
public:
  InsertRowAction(GetCatalogRootPageIdPort &getCatalogRootPageIdPort,
                  GetCatalogTableByNamePort &getCatalogTableByNamePort,
                  UpdateCatalogTablePort &updateCatalogTablePort,
                  CreateSpaceMapPagePort &createSpaceMapPagePort,
                  CalculateRequiredSpacePort &calculateRequiredSpacePort,
                  FindPageWithSpacePort &findPageWithSpacePort,
                  CreateTuplePort &createTuplePort,
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
  CreateTuplePort &createTuplePort;
  UpdateFreeSpacePort &updateFreeSpacePort;
};

} // namespace RdbService
