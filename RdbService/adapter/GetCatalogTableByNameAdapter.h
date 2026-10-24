#pragma once

#include "../../CatalogService/actions/GetTableByNameAction.h"
#include "../port/GetCatalogTableByNamePort.h"

class GetCatalogTableByNameAdapter final : public GetCatalogTableByNamePort {
public:
  explicit GetCatalogTableByNameAdapter(GetTableByNameAction &action);

  RdbTableDefinition getTableByName(uint32_t catalogRootPageId,
                                    const std::string &tableName) override;

private:
  GetTableByNameAction &action;
};
