#pragma once
#include "../../CatalogService/actions/DropTableAction.h"
#include "../../CatalogService/actions/GetTableByNameAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include "../../TupleService/actions/DropTupleTableAction.h"
#include "../port/DropTablePort.h"
namespace unidb::rdb {
class CatalogDropTableAdapter final : public DropTablePort {
public:
  CatalogDropTableAdapter(::GetCatalogRootPageIdAction &getRoot,
                          ::GetTableByNameAction &getTable,
                          ::DropTableAction &dropTable,
                          ::DropTupleTableAction &dropTupleTable)
      : getRoot(getRoot), getTable(getTable), dropTable(dropTable),
        dropTupleTable(dropTupleTable) {}
  void drop(const std::string &tableName) override;

private:
  ::GetCatalogRootPageIdAction &getRoot;
  ::GetTableByNameAction &getTable;
  ::DropTableAction &dropTable;
  ::DropTupleTableAction &dropTupleTable;
};
} // namespace unidb::rdb