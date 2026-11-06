#pragma once
#include "../../CatalogService/actions/GetAllTablesAction.h"
#include "../port/GetCatalogTablesPort.h"
class GetCatalogTablesAdapter final : public GetCatalogTablesPort {
public:
  explicit GetCatalogTablesAdapter(GetAllTablesAction &);
  std::vector<RdbTableDefinition> getTables(uint32_t rootPageId) override;
private:
  GetAllTablesAction &action;
};
