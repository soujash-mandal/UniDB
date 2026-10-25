#include "InsertRowAction.h"

#include <stdexcept>
#include <utility>

namespace RdbService {

InsertRowAction::InsertRowAction(
    GetCatalogRootPageIdPort &getCatalogRootPageIdPort,
    GetCatalogTableByNamePort &getCatalogTableByNamePort,
    UpdateCatalogTablePort &updateCatalogTablePort,
    CreateSpaceMapPagePort &createSpaceMapPagePort,
    CalculateRequiredSpacePort &calculateRequiredSpacePort,
    FindPageWithSpacePort &findPageWithSpacePort,
    CreateTupleWithFreeSpacePort &createTupleWithFreeSpacePort,
    UpdateFreeSpacePort &updateFreeSpacePort)
    : getCatalogRootPageIdPort(getCatalogRootPageIdPort),
      getCatalogTableByNamePort(getCatalogTableByNamePort),
      updateCatalogTablePort(updateCatalogTablePort),
      createSpaceMapPagePort(createSpaceMapPagePort),
      calculateRequiredSpacePort(calculateRequiredSpacePort),
      findPageWithSpacePort(findPageWithSpacePort),
      createTupleWithFreeSpacePort(createTupleWithFreeSpacePort),
      updateFreeSpacePort(updateFreeSpacePort) {}

InsertedRowLocation InsertRowAction::execute(uint32_t metadataPageId,
                                             const std::string &tableName,
                                             std::vector<char> tupleData) {
  if (tableName.empty()) {
    throw std::invalid_argument("Table name must not be empty");
  }

  const uint32_t requiredSpace =
      calculateRequiredSpacePort.calculateRequiredSpace(tupleData);
  const uint32_t catalogRootPageId =
      getCatalogRootPageIdPort.getCatalogRootPageId(metadataPageId);
  RdbTableDefinition table =
      getCatalogTableByNamePort.getTableByName(catalogRootPageId, tableName);

  const bool createdSpaceMapRoot =
      table.firstSpaceMapPageId == std::numeric_limits<uint32_t>::max();
  if (createdSpaceMapRoot) {
    table.firstSpaceMapPageId = createSpaceMapPagePort.createSpaceMapPage();
    // Publish the root first so failed discovery cannot orphan the page.
    updateCatalogTablePort.updateTable(catalogRootPageId, table,
                                       table.firstSpaceMapPageId);
  }

  const RdbFindPageWithSpaceResult target =
      findPageWithSpacePort.findPageWithSpace(table.firstSpaceMapPageId,
                                              requiredSpace);
  const RdbCreateTupleWithFreeSpaceResult insertResult =
      createTupleWithFreeSpacePort.createTupleWithFreeSpace(
          target.tuplePageId, std::move(tupleData));
  updateFreeSpacePort.updateFreeSpace(target.spaceMapPageId, target.tuplePageId,
                                      insertResult.remainingFreeSpace);
  return {target.tuplePageId, insertResult.slotId};
}

} // namespace RdbService
