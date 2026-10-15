#pragma once
#include "../port/GetTableDefinitionPort.h"
#include "../../CatalogService/actions/GetCatalogTableDefinitionAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
namespace unidb::rdb {
class CatalogGetTableDefinitionAdapter final : public GetTableDefinitionPort {
public:
  CatalogGetTableDefinitionAdapter(::GetCatalogRootPageIdAction &, ::GetCatalogTableDefinitionAction &);
  TableDefinition get(const std::string &tableName) override;
private:
  ::GetCatalogRootPageIdAction &getRoot;
  ::GetCatalogTableDefinitionAction &getTable;
};
}