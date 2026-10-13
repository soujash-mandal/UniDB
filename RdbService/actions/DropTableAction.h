#pragma once
#include "../../CatalogService/actions/DropTableAction.h"
#include "../../CatalogService/actions/GetTableByNameAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include <string>
namespace unidb::rdb {
class DropTableAction {
public:
  DropTableAction(::GetCatalogRootPageIdAction &getRoot,
                  ::GetTableByNameAction &getTable,
                  ::DropTableAction &dropTable)
      : getRoot(getRoot), getTable(getTable), dropTable(dropTable) {}
  void execute(const std::string &tableName);
private:
  ::GetCatalogRootPageIdAction &getRoot;
  ::GetTableByNameAction &getTable;
  ::DropTableAction &dropTable;
};
}
