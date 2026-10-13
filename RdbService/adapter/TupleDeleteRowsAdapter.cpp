#include "TupleDeleteRowsAdapter.h"
#include "RdbTupleCodec.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace unidb::rdb {
TupleDeleteRowsAdapter::TupleDeleteRowsAdapter(::GetCatalogRootPageIdAction &root, ::GetTableByNameAction &table, ::ScanTupleTableWithLocationsAction &scanAction, ::DeleteTupleAction &deleteAction)
  : getRoot(root), getTable(table), scan(scanAction), deleteTuple(deleteAction) {}
std::size_t TupleDeleteRowsAdapter::erase(const std::string &tableName, const unidb::sql::WhereEquals &condition) {
  constexpr CatalogPageId metadataPageId = 1;
  const auto root = getRoot.execute(metadataPageId);
  const auto table = getTable.execute(root, tableName);
  auto column = std::find_if(table.columns.begin(), table.columns.end(), [&](const Column &candidate) { return candidate.name == condition.columnName; });
  if (column == table.columns.end()) throw std::invalid_argument("Unknown delete predicate column: " + condition.columnName);
  const auto index = static_cast<std::size_t>(column - table.columns.begin());
  const bool intColumn = column->type == DataType::INT;
  if ((intColumn && !std::holds_alternative<std::int64_t>(condition.value)) ||
      (column->type == DataType::VARCHAR && !std::holds_alternative<std::string>(condition.value))) {
    throw std::invalid_argument("Predicate value type does not match column type");
  }
  if (column->type != DataType::INT && column->type != DataType::VARCHAR) {
    throw std::invalid_argument("Column type is not supported by the V1 SQL contract");
  }
  if (table.firstTuplePageId == std::numeric_limits<CatalogPageId>::max()) return 0;

  std::vector<unidb::sql::SqlType> schema;
  schema.reserve(table.columns.size());
  for (const auto &entry : table.columns) {
    switch (entry.type) {
    case DataType::INT: schema.push_back(unidb::sql::SqlType::Int); break;
    case DataType::VARCHAR: schema.push_back(unidb::sql::SqlType::Text); break;
    default: throw std::invalid_argument("Column type is not supported by the V1 SQL contract");
    }
  }

  std::vector<LocatedTuple> matches;
  for (const auto &record : scan.execute(table.firstTuplePageId)) {
    const auto values = RdbTupleCodec::decode(schema, record.data);
    if (values[index] == condition.value) matches.push_back(record);
  }
  for (const auto &record : matches) deleteTuple.execute(record.pageId, record.slotId);
  return matches.size();
}
}
