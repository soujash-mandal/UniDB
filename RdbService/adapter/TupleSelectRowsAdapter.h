#pragma once
#include "../port/SelectRowsPort.h"
#include "../../CatalogService/actions/GetCatalogTableDefinitionAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include "../../TupleService/actions/ScanTupleTableAction.h"
namespace unidb::rdb {
class TupleSelectRowsAdapter final : public SelectRowsPort {
public:
  TupleSelectRowsAdapter(::GetCatalogRootPageIdAction &,
                         ::GetCatalogTableDefinitionAction &,
                         ::ScanTupleTableAction &);
  std::vector<std::vector<unidb::sql::SqlValue>> select(
      const unidb::sql::SelectStatement &) override;
private:
  ::GetCatalogRootPageIdAction &getRoot;
  ::GetCatalogTableDefinitionAction &getTable;
  ::ScanTupleTableAction &scan;
};
}
