#pragma once
#include "../port/DeleteRowsPort.h"
#include "../../CatalogService/actions/GetCatalogTableDefinitionAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include "../../TupleService/actions/DeleteTupleAction.h"
#include "../../TupleService/actions/ScanTupleTableWithLocationsAction.h"
namespace unidb::rdb {
class TupleDeleteRowsAdapter final : public DeleteRowsPort {
public:
  TupleDeleteRowsAdapter(::GetCatalogRootPageIdAction &,
                         ::GetCatalogTableDefinitionAction &,
                         ::ScanTupleTableWithLocationsAction &,
                         ::DeleteTupleAction &);
  std::size_t erase(const std::string &, const unidb::sql::WhereEquals &) override;
private:
  ::GetCatalogRootPageIdAction &getRoot;
  ::GetCatalogTableDefinitionAction &getTable;
  ::ScanTupleTableWithLocationsAction &scan;
  ::DeleteTupleAction &deleteTuple;
};
}
