#pragma once
#include "../../CatalogService/actions/CreateTableAction.h"
#include "../../RdbMetadataService/actions/AllocateNextTableIdAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include "../../SQLParserContract/sqlStatement.h"
namespace unidb::rdb {
class CreateTableAction {
public:
  CreateTableAction(::GetCatalogRootPageIdAction &getRoot,
                    ::AllocateNextTableIdAction &allocateId,
                    ::CreateTableAction &createTable)
      : getRoot(getRoot), allocateId(allocateId), createTable(createTable) {}
  CatalogTableId execute(const unidb::sql::CreateTableStatement &statement);
private:
  ::GetCatalogRootPageIdAction &getRoot;
  ::AllocateNextTableIdAction &allocateId;
  ::CreateTableAction &createTable;
};
}
