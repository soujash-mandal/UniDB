#include "InsertRowAction.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace RdbService {

// Step 1: Receive and keep references to the Ports this Action needs.
// Each Port is an RDBService-owned interface; its Adapter talks to the
// corresponding lower-level service Action.
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

// Insert one encoded row into the named table and return its page/slot location.
InsertedRowLocation InsertRowAction::execute(uint32_t metadataPageId,
                                             const std::string &tableName,
                                             std::vector<char> tupleData) {
  // Step 2: Validate the table name before calling any dependency.
  if (tableName.empty()) {
    throw std::invalid_argument("Table name must not be empty");
  }

  // Step 3: Calculate how much free space the row needs, including its slot
  // entry. SpaceMap uses this value to choose a page that can fit the row.
  const uint32_t requiredSpace =
      calculateRequiredSpacePort.calculateRequiredSpace(tupleData);

  // Step 4: Read the catalog root page ID stored in the RDB metadata page.
  const uint32_t catalogRootPageId =
      getCatalogRootPageIdPort.getCatalogRootPageId(metadataPageId);

  // Step 5: Look up the requested table in the catalog. Its definition tells
  // us which SpaceMap root tracks the table's tuple pages.
  RdbTableDefinition table =
      getCatalogTableByNamePort.getTableByName(catalogRootPageId, tableName);

  // Step 6: The maximum uint32_t value is the sentinel meaning that this table
  // does not have a SpaceMap root yet.
  const bool createdSpaceMapRoot =
      table.firstSpaceMapPageId == std::numeric_limits<uint32_t>::max();
  if (createdSpaceMapRoot) {
    // Step 7: Create the table's first SpaceMap page.
    table.firstSpaceMapPageId = createSpaceMapPagePort.createSpaceMapPage();

    // Step 8: Persist the new root ID in the catalog before using it. This
    // ensures the catalog points to the root if later page discovery fails.
    updateCatalogTablePort.updateTable(catalogRootPageId, table,
                                       table.firstSpaceMapPageId);
  }

  // Step 9: Ask SpaceMap to find a tuple page with enough room. The result
  // includes both the tuple page ID and the SpaceMap page that owns its entry.
  const RdbFindPageWithSpaceResult target =
      findPageWithSpacePort.findPageWithSpace(table.firstSpaceMapPageId,
                                              requiredSpace);

  // Step 10: Insert the row into the selected tuple page. The returned result
  // is expected to contain the new slot ID and the page's actual remaining
  // free space after the insertion.
  const RdbCreateTupleWithFreeSpaceResult insertResult =
      createTupleWithFreeSpacePort.createTupleWithFreeSpace(
          target.tuplePageId, std::move(tupleData));

  // Step 11: Refresh SpaceMap's recorded free-space value using the actual
  // post-insert value, not the estimate calculated in Step 3.
  updateFreeSpacePort.updateFreeSpace(target.spaceMapPageId, target.tuplePageId,
                                      insertResult.remainingFreeSpace);

  // Step 12: Return the physical location of the inserted row to the caller.
  return {target.tuplePageId, insertResult.slotId};
}

} // namespace RdbService
