#pragma once
#include "../../CatalogService/actions/GetTableByNameAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include "../../TupleService/actions/ScanTupleTableAction.h"
#include "../port/SelectRowsPort.h"
namespace unidb::rdb {
class TupleSelectRowsAdapter final : public SelectRowsPort {
public:
  TupleSelectRowsAdapter(::GetCatalogRootPageIdAction &,
                         ::GetTableByNameAction &, ::ScanTupleTableAction &);
  std::vector<std::vector<unidb::sql::SqlValue>>
  select(const unidb::sql::SelectStatement &) override;

private:
  ::GetCatalogRootPageIdAction &getRoot;
  ::GetTableByNameAction &getTable;
  ::ScanTupleTableAction &scan;
};
} // namespace unidb::rdb
