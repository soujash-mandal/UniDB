#pragma once
#include "../../CatalogService/actions/GetTableByNameAction.h"
#include "../../CatalogService/actions/UpdateTableAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include "../../TupleService/actions/InsertTupleIntoTableAction.h"
#include "../../SQLParserContract/sqlStatement.h"
namespace unidb::rdb {
class InsertRowAction {
public:
  InsertRowAction(::GetCatalogRootPageIdAction &getRoot,
                  ::GetTableByNameAction &getTable,
                  ::UpdateTableAction &updateTable,
                  ::InsertTupleIntoTableAction &insertTuple)
      : getRoot(getRoot), getTable(getTable), updateTable(updateTable), insertTuple(insertTuple) {}
  void execute(const unidb::sql::InsertStatement &statement);
private:
  ::GetCatalogRootPageIdAction &getRoot;
  ::GetTableByNameAction &getTable;
  ::UpdateTableAction &updateTable;
  ::InsertTupleIntoTableAction &insertTuple;
};
}
