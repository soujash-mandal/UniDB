#include "SelectAllRowsAction.h"

#include "../domain/RdbTable.h"

#include <iterator>
#include <limits>
#include <stdexcept>

namespace RdbService {

SelectAllRowsAction::SelectAllRowsAction(
    GetCatalogRootPageIdPort &getCatalogRootPageIdPort,
    GetCatalogTableByNamePort &getCatalogTableByNamePort,
    GetTablePagesPort &getTablePagesPort,
    GetAllTuplesPort &getAllTuplesPort)
    : getCatalogRootPageIdPort(getCatalogRootPageIdPort),
      getCatalogTableByNamePort(getCatalogTableByNamePort),
      getTablePagesPort(getTablePagesPort), getAllTuplesPort(getAllTuplesPort) {}

std::vector<std::vector<char>>
SelectAllRowsAction::execute(uint32_t metadataPageId,
                             const std::string &tableName) {
  if (tableName.empty()) {
    throw std::invalid_argument("Table name must not be empty");
  }

  const uint32_t catalogRootPageId =
      getCatalogRootPageIdPort.getCatalogRootPageId(metadataPageId);
  const RdbTableDefinition table =
      getCatalogTableByNamePort.getTableByName(catalogRootPageId, tableName);

  if (table.firstSpaceMapPageId == std::numeric_limits<uint32_t>::max()) {
    return {};
  }

  const auto tuplePageIds =
      getTablePagesPort.getTablePages(table.firstSpaceMapPageId);
  std::vector<std::vector<char>> rows;
  for (const uint32_t tuplePageId : tuplePageIds) {
    auto pageRows = getAllTuplesPort.getAllTuples(tuplePageId);
    rows.insert(rows.end(), std::make_move_iterator(pageRows.begin()),
                std::make_move_iterator(pageRows.end()));
  }
  return rows;
}

} // namespace RdbService
