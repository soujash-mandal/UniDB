#include "InsertRowAction.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace RdbService {
namespace {
constexpr uint32_t TUPLE_SLOT_ENTRY_BYTES = 2 * sizeof(uint32_t);
constexpr uint32_t TUPLE_PAGE_SIZE_BYTES = 8192;
constexpr uint32_t TUPLE_PAGE_HEADER_BYTES = 2 * sizeof(uint32_t);
constexpr uint32_t MAX_TUPLE_BYTES = TUPLE_PAGE_SIZE_BYTES -
                                     TUPLE_PAGE_HEADER_BYTES -
                                     TUPLE_SLOT_ENTRY_BYTES;
}

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
  if (tupleData.size() >
      std::numeric_limits<uint32_t>::max() - TUPLE_SLOT_ENTRY_BYTES) {
    throw std::invalid_argument("Tuple exceeds the V1 tuple size limit");
  }

  const uint32_t catalogRootPageId =
      getCatalogRootPageIdPort.getCatalogRootPageId(metadataPageId);
  RdbTableDefinition table =
      getCatalogTableByNamePort.getTableByName(catalogRootPageId, tableName);

  if (table.firstSpaceMapPageId == std::numeric_limits<uint32_t>::max()) {
    const uint32_t rootSpaceMapPageId =
        createSpaceMapPagePort.createSpaceMapPage();
    updateCatalogTablePort.updateTable(catalogRootPageId, table,
                                       rootSpaceMapPageId);
    table.firstSpaceMapPageId = rootSpaceMapPageId;
  }

  const uint32_t requiredSpace =
      static_cast<uint32_t>(tupleData.size()) + TUPLE_SLOT_ENTRY_BYTES;
  const RdbFindPageWithSpaceResult target =
      findPageWithSpacePort.findPageWithSpace(table.firstSpaceMapPageId,
                                              requiredSpace);
  const uint32_t slotId =
      createTuplePort.createTuple(target.tuplePageId, std::move(tupleData));
  return {target.tuplePageId, slotId};
}

} // namespace RdbService
