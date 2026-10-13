#pragma once
#include "../port/InsertRowPort.h"
#include "../../CatalogService/actions/GetTableByNameAction.h"
#include "../../CatalogService/actions/UpdateTableAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include "../../TupleService/actions/InsertTupleIntoTableAction.h"
namespace unidb::rdb {
class TupleInsertRowAdapter final : public InsertRowPort {
public:
  TupleInsertRowAdapter(::GetCatalogRootPageIdAction &, ::GetTableByNameAction &, ::UpdateTableAction &, ::InsertTupleIntoTableAction &);
  void insert(const unidb::sql::InsertStatement &) override;
private:
  ::GetCatalogRootPageIdAction &getRoot; ::GetTableByNameAction &getTable; ::UpdateTableAction &updateTable; ::InsertTupleIntoTableAction &insertTuple;
};
}
