#pragma once
#include "../../CatalogService/actions/GetTableByNameAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include "../../TupleService/actions/ScanTupleTableAction.h"
#include "../../SQLParserContract/sqlStatement.h"
namespace unidb::rdb {
class SelectRowsAction {
public:
  SelectRowsAction(::GetCatalogRootPageIdAction &getRoot, ::GetTableByNameAction &getTable, ::ScanTupleTableAction &scan) : getRoot(getRoot), getTable(getTable), scan(scan) {}
  std::vector<std::vector<unidb::sql::SqlValue>> execute(const unidb::sql::SelectStatement &statement);
private:
  ::GetCatalogRootPageIdAction &getRoot;
  ::GetTableByNameAction &getTable;
  ::ScanTupleTableAction &scan;
};
}
