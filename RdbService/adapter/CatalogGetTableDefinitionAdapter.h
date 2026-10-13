#pragma once
#include "../port/GetTableDefinitionPort.h"
#include "../../CatalogService/actions/GetTableByNameAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
namespace unidb::rdb {
class CatalogGetTableDefinitionAdapter final : public GetTableDefinitionPort {
public:
  CatalogGetTableDefinitionAdapter(::GetCatalogRootPageIdAction &, ::GetTableByNameAction &);
  TableDefinition get(const std::string &tableName) override;
private:
  ::GetCatalogRootPageIdAction &getRoot;
  ::GetTableByNameAction &getTable;
};
}