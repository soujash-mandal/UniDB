#pragma once

#include "../../CatalogService/actions/GetTableByNameAction.h"
#include "../domain/RdbTable.h"

#include <string>

class GetCatalogTableByNameAdapter final {
public:
  explicit GetCatalogTableByNameAdapter(GetTableByNameAction &action);
  RdbTableDefinition getTableByName(uint32_t catalogRootPageId,
                                    const std::string &tableName);
private:
  GetTableByNameAction &action;
};
