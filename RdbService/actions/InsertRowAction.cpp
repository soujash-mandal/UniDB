#include "InsertRowAction.h"

#include <stdexcept>
#include <utility>

namespace RdbService {
namespace {
constexpr uint32_t TUPLE_SLOT_ENTRY_BYTES = 2 * sizeof(uint32_t);
constexpr uint32_t TUPLE_PAGE_SIZE_BYTES = 8192;
constexpr uint32_t TUPLE_PAGE_HEADER_BYTES = 2 * sizeof(uint32_t);
constexpr uint32_t MAX_TUPLE_BYTES =
    TUPLE_PAGE_SIZE_BYTES - TUPLE_PAGE_HEADER_BYTES - TUPLE_SLOT_ENTRY_BYTES;
} // namespace

InsertRowAction::InsertRowAction(
    GetCatalogRootPageIdPort &getCatalogRootPageIdPort,
    GetCatalogTableByNamePort &getCatalogTableByNamePort,
    UpdateCatalogTablePort &updateCatalogTablePort,
    CreateSpaceMapPagePort &createSpaceMapPagePort,
    FindPageWithSpacePort &findPageWithSpacePort,
    CreateTuplePort &createTuplePort)
    : getCatalogRootPageIdPort(getCatalogRootPageIdPort),
      getCatalogTableByNamePort(getCatalogTableByNamePort),
      updateCatalogTablePort(updateCatalogTablePort),
      createSpaceMapPagePort(createSpaceMapPagePort),
      findPageWithSpacePort(findPageWithSpacePort),
      createTuplePort(createTuplePort) {}

InsertedRowLocation InsertRowAction::execute(uint32_t metadataPageId,
                                             const std::string &tableName,
                                             std::vector<char> tupleData) {
  if (tableName.empty()) {
    throw std::invalid_argument("Table name must not be empty");
  }
  if (tupleData.size() > MAX_TUPLE_BYTES) {
    throw std::invalid_argument("Tuple exceeds the V1 tuple size limit");
  }

  const uint32_t catalogRootPageId =
      getCatalogRootPageIdPort.getCatalogRootPageId(metadataPageId);
  RdbTableDefinition table =
      getCatalogTableByNamePort.getTableByName(catalogRootPageId, tableName);

  const bool createdSpaceMapRoot =
      table.firstSpaceMapPageId == std::numeric_limits<uint32_t>::max();
  if (createdSpaceMapRoot) {
    table.firstSpaceMapPageId = createSpaceMapPagePort.createSpaceMapPage();
  }

  const uint32_t requiredSpace =
      static_cast<uint32_t>(tupleData.size()) + TUPLE_SLOT_ENTRY_BYTES;
  const RdbFindPageWithSpaceResult target =
      findPageWithSpacePort.findPageWithSpace(table.firstSpaceMapPageId,
                                              requiredSpace);
  // Persist a newly discovered root only after page discovery succeeds, but
  // before writing the tuple so a catalog-write failure cannot hide an insert.
  if (createdSpaceMapRoot) {
    updateCatalogTablePort.updateTable(catalogRootPageId, table,
                                       table.firstSpaceMapPageId);
  }

  const uint32_t slotId =
      createTuplePort.createTuple(target.tuplePageId, std::move(tupleData));
  return {target.tuplePageId, slotId};
}

} // namespace RdbService
