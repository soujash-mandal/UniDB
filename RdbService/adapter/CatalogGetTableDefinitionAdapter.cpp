#include "CatalogGetTableDefinitionAdapter.h"
#include <stdexcept>
namespace unidb::rdb {
CatalogGetTableDefinitionAdapter::CatalogGetTableDefinitionAdapter(
    ::GetCatalogRootPageIdAction &root, ::GetTableByNameAction &table)
    : getRoot(root), getTable(table) {}
TableDefinition
CatalogGetTableDefinitionAdapter::get(const std::string &tableName) {
  constexpr CatalogPageId metadataPageId = 1;
  const auto root = getRoot.execute(metadataPageId);
  const auto table = getTable.execute(root, tableName);
  TableDefinition result;
  result.tableId = table.tableId;
  result.tableName = table.name;
  result.columns.reserve(table.columns.size());
  for (const auto &column : table.columns) {
    switch (column.type) {
    case DataType::INT:
    case DataType::BIGINT:
      result.columns.push_back({column.name, unidb::sql::SqlType::Int});
      break;
    case DataType::VARCHAR:
      result.columns.push_back({column.name, unidb::sql::SqlType::Text});
      break;
    default:
      throw std::runtime_error(
          "Column type is not supported by the V1 SQL contract");
    }
  }
  return result;
}
} // namespace unidb::rdb