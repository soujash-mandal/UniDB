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
    // Step 2.1: Stop immediately for an empty name; no Port has been called.
    throw std::invalid_argument("Table name must not be empty");
  }
  // Step 2.2: If the name is valid, continue with space calculation.

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

  // Step 6: Decide whether this table already has a SpaceMap root.
  const bool createdSpaceMapRoot =
      table.firstSpaceMapPageId == std::numeric_limits<uint32_t>::max();
  if (createdSpaceMapRoot) {
    // Step 6.1 (TRUE branch): No SpaceMap root exists, so create the first one.
    table.firstSpaceMapPageId = createSpaceMapPagePort.createSpaceMapPage();

    // Step 6.2: Save the newly created root ID in the catalog before proceeding.
    // This way, if page discovery fails later, the catalog still records the root.
    updateCatalogTablePort.updateTable(catalogRootPageId, table,
                                       table.firstSpaceMapPageId);
  } else {
    // Step 6.3 (FALSE branch): A root already exists; reuse it without creating
    // a page or updating the catalog root field.
  }
  // Step 6.4: Both branches rejoin here and use table.firstSpaceMapPageId.

  // Step 7: Ask SpaceMap to find a tuple page with enough room. The result
  // includes both the tuple page ID and the SpaceMap page that owns its entry.
  const RdbFindPageWithSpaceResult target =
      findPageWithSpacePort.findPageWithSpace(table.firstSpaceMapPageId,
                                              requiredSpace);

  // Step 8: Insert the row into the selected tuple page. The returned result
  // is expected to contain the new slot ID and the page's actual remaining
  // free space after the insertion.
  const RdbCreateTupleWithFreeSpaceResult insertResult =
      createTupleWithFreeSpacePort.createTupleWithFreeSpace(
          target.tuplePageId, std::move(tupleData));

  // Step 9: Refresh SpaceMap's recorded free-space value using the actual
  // post-insert value, not the estimate calculated in Step 3.
  updateFreeSpacePort.updateFreeSpace(target.spaceMapPageId, target.tuplePageId,
                                      insertResult.remainingFreeSpace);

  // Step 10: Return the physical location of the inserted row to the caller.
  return {target.tuplePageId, insertResult.slotId};
}

} // namespace RdbService
