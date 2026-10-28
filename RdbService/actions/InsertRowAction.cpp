#include "InsertRowAction.h"

#include <cstddef>
#include <exception>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
constexpr std::size_t TUPLE_PAGE_SIZE_BYTES = 8192;
constexpr std::size_t TUPLE_PAGE_HEADER_BYTES = 8;
constexpr std::size_t TUPLE_SLOT_ENTRY_BYTES = 8;
constexpr std::size_t MAX_TUPLE_BYTES =
    TUPLE_PAGE_SIZE_BYTES - TUPLE_PAGE_HEADER_BYTES - TUPLE_SLOT_ENTRY_BYTES;
} // namespace

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
    CreateTuplePort &createTuplePort, UpdateFreeSpacePort &updateFreeSpacePort)
    : getCatalogRootPageIdPort(getCatalogRootPageIdPort),
      getCatalogTableByNamePort(getCatalogTableByNamePort),
      updateCatalogTablePort(updateCatalogTablePort),
      createSpaceMapPagePort(createSpaceMapPagePort),
      calculateRequiredSpacePort(calculateRequiredSpacePort),
      findPageWithSpacePort(findPageWithSpacePort),
      createTuplePort(createTuplePort),
      updateFreeSpacePort(updateFreeSpacePort) {}

// Insert one encoded row into the named table and return its page/slot
// location.
InsertedRowLocation InsertRowAction::execute(uint32_t metadataPageId,
                                             const std::string &tableName,
                                             std::vector<char> tupleData) {
  // Step 2: Validate the table name before calling any dependency.
  if (tableName.empty()) {
    // Step 2.1: Stop immediately for an empty name; no Port has been called.
    throw std::invalid_argument("Table name must not be empty");
  }
  // Step 2.2: If the name is valid, continue with input validation.
  // Step 2.3: Reject payloads larger than one empty tuple page can hold,
  // before calling any Port. V1 uses an 8-byte page header and 8-byte slot.
  if (tupleData.size() > MAX_TUPLE_BYTES) {
    throw std::invalid_argument("Tuple exceeds the V1 tuple size limit");
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

  // Step 6: Decide whether this table already has a SpaceMap root.
  const bool createdSpaceMapRoot =
      table.firstSpaceMapPageId == std::numeric_limits<uint32_t>::max();
  if (createdSpaceMapRoot) {
    // Step 6.1 (TRUE branch): No SpaceMap root exists, so create the first one.
    table.firstSpaceMapPageId = createSpaceMapPagePort.createSpaceMapPage();

    // Step 6.2: Save the newly created root ID in the catalog before
    // proceeding. This way, if page discovery fails later, the catalog still
    // records the root.
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
  const CreateTupleResult insertResult =
      createTuplePort.createTuple(target.tuplePageId, std::move(tupleData));

  // Step 9: Refresh SpaceMap's recorded free-space value using the actual
  // post-insert value, not the estimate calculated in Step 3.
  bool spaceMapUpdated = true;
  std::string spaceMapUpdateError;
  try {
    updateFreeSpacePort.updateFreeSpace(
        target.spaceMapPageId, target.tuplePageId, insertResult.freeSpace);
  } catch (const std::exception &error) {
    // The tuple is already persisted. Return its committed location instead
    // of throwing an error that could cause the caller to insert it again.
    spaceMapUpdated = false;
    spaceMapUpdateError = error.what();
  } catch (...) {
    // Preserve the committed outcome even for a non-standard adapter error.
    spaceMapUpdated = false;
    spaceMapUpdateError = "Unknown SpaceMap free-space update failure";
  }

  // Step 10: Return the committed row location and enough metadata to retry
  // the SpaceMap update if spaceMapUpdated is false.
  return {target.tuplePageId,    insertResult.slotId,
          target.spaceMapPageId, insertResult.freeSpace,
          spaceMapUpdated,       std::move(spaceMapUpdateError)};
}

} // namespace RdbService
