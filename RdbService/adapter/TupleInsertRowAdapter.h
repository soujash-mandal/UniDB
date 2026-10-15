#pragma once
#include "../port/InsertRowPort.h"
#include "../../CatalogService/actions/GetCatalogTableDefinitionAction.h"
#include "../../CatalogService/actions/UpdateCatalogTupleRootAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include "../../TupleService/actions/InsertTupleIntoTableAction.h"
namespace unidb::rdb {
class TupleInsertRowAdapter final : public InsertRowPort {
public:
  TupleInsertRowAdapter(::GetCatalogRootPageIdAction &,
                        ::GetCatalogTableDefinitionAction &,
                        ::UpdateCatalogTupleRootAction &,
                        ::InsertTupleIntoTableAction &);
  void insert(const unidb::sql::InsertStatement &) override;
private:
  ::GetCatalogRootPageIdAction &getRoot;
  ::GetCatalogTableDefinitionAction &getTable;
  ::UpdateCatalogTupleRootAction &updateTupleRoot;
  ::InsertTupleIntoTableAction &insertTuple;
};
}
