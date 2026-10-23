#include "CatalogDropTableAdapter.h"
namespace unidb::rdb {
void CatalogDropTableAdapter::drop(const std::string &tableName) {
  constexpr CatalogPageId metadataPageId = 1;
  const auto root = getRoot.execute(metadataPageId);
  const auto table = getTable.execute(root, tableName);
  dropTupleTable.execute(table.firstTuplePageId);\n  dropTable.execute(root, table.tableId);
}
} // namespace unidb::rdb
