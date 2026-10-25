#pragma once

#include "../port/GetAllTuplesPort.h"
#include "../port/GetCatalogRootPageIdPort.h"
#include "../port/GetCatalogTableByNamePort.h"
#include "../port/GetTablePagesPort.h"

#include <cstdint>
#include <string>
#include <vector>

namespace RdbService {

class SelectAllRowsAction {
public:
  SelectAllRowsAction(GetCatalogRootPageIdPort &getCatalogRootPageIdPort,
                      GetCatalogTableByNamePort &getCatalogTableByNamePort,
                      GetTablePagesPort &getTablePagesPort,
                      GetAllTuplesPort &getAllTuplesPort);

  std::vector<std::vector<char>> execute(uint32_t metadataPageId,
                                         const std::string &tableName);

private:
  GetCatalogRootPageIdPort &getCatalogRootPageIdPort;
  GetCatalogTableByNamePort &getCatalogTableByNamePort;
  GetTablePagesPort &getTablePagesPort;
  GetAllTuplesPort &getAllTuplesPort;
};

} // namespace RdbService
