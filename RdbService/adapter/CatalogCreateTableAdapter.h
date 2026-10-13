#pragma once
#include "../port/CreateTablePort.h"
#include "../../CatalogService/actions/CreateTableAction.h"
#include "../../RdbMetadataService/actions/AllocateNextTableIdAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
namespace unidb::rdb {
class CatalogCreateTableAdapter final : public CreateTablePort {
public:
  CatalogCreateTableAdapter(::GetCatalogRootPageIdAction &getRoot,
                            ::AllocateNextTableIdAction &allocateId,
                            ::CreateTableAction &createTable)
      : getRoot(getRoot), allocateId(allocateId), createTable(createTable) {}
  uint32_t create(const unidb::sql::CreateTableStatement &statement) override;
private:
  ::GetCatalogRootPageIdAction &getRoot;
  ::AllocateNextTableIdAction &allocateId;
  ::CreateTableAction &createTable;
};
}