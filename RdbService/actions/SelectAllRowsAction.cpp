#include "SelectAllRowsAction.h"

#include "../domain/RdbRowCodec.h"

#include <stdexcept>

SelectAllRowsAction::SelectAllRowsAction(RdbMetadataPort &metadataPort,
                                         CatalogTablePort &catalogTablePort,
                                         SpaceMapPort &spaceMapPort,
                                         TuplePagePort &tuplePagePort)
    : metadataPort(metadataPort), catalogTablePort(catalogTablePort),
      spaceMapPort(spaceMapPort), tuplePagePort(tuplePagePort) {}

std::vector<RdbRow> SelectAllRowsAction::execute(const std::string &tableName) {
  if (tableName.empty()) {
    throw std::invalid_argument("Table name cannot be empty");
  }

  const CatalogPageId catalogRootPageId = metadataPort.getCatalogRootPageId();
  const RdbTableDefinition table =
      catalogTablePort.getTableByName(catalogRootPageId, tableName);
  if (table.firstSpaceMapPageId == UINT32_MAX) {
    return {};
  }

  std::vector<RdbRow> rows;
  const auto tuplePageIds =
      spaceMapPort.getTablePages(table.firstSpaceMapPageId);
  for (const uint32_t tuplePageId : tuplePageIds) {
    const auto tuples = tuplePagePort.getAllTuples(tuplePageId);
    for (const auto &tuple : tuples) {
      rows.push_back(RdbRowCodec::decode(table, tuple));
    }
  }
  return rows;
}
