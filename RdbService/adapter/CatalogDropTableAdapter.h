#pragma once
#include "../port/DropTablePort.h"
#include "../../CatalogService/actions/DropTableAction.h"
#include "../../CatalogService/actions/GetCatalogTableDefinitionAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
namespace unidb::rdb {
class CatalogDropTableAdapter final : public DropTablePort {
public:
  CatalogDropTableAdapter(::GetCatalogRootPageIdAction &getRoot,
                          ::GetCatalogTableDefinitionAction &getTable,
                          ::DropTableAction &dropTable)
      : getRoot(getRoot), getTable(getTable), dropTable(dropTable) {}
  void drop(const std::string &tableName) override;
private:
  ::GetCatalogRootPageIdAction &getRoot;
  ::GetCatalogTableDefinitionAction &getTable;
  ::DropTableAction &dropTable;
};
}
