#include "DropTableAction.h"
namespace unidb::rdb {
void DropTableAction::execute(const std::string &tableName) {
  constexpr CatalogPageId metadataPageId = 1;
  const auto root = getRoot.execute(metadataPageId);
  const auto table = getTable.execute(root, tableName);
  dropTable.execute(root, table.tableId);
}
}
