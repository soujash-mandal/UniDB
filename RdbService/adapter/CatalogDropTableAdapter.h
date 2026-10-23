#pragma once
#include "../../CatalogService/actions/DropTableAction.h"
#include "../../CatalogService/actions/GetTableByNameAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include "../port/DropTablePort.h"
namespace unidb::rdb {
class CatalogDropTableAdapter final : public DropTablePort {
public:
  CatalogDropTableAdapter(::GetCatalogRootPageIdAction &getRoot,
                          ::GetTableByNameAction &getTable,
                          ::DropTableAction &dropTable)
      : getRoot(getRoot), getTable(getTable), dropTable(dropTable) {}
  void drop(const std::string &tableName) override;

private:
  ::GetCatalogRootPageIdAction &getRoot;
  ::GetTableByNameAction &getTable;
  ::DropTableAction &dropTable;
};
} // namespace unidb::rdb