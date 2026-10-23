#include "TupleDeleteRowsAdapter.h"
#include "RdbTupleCodec.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace unidb::rdb {
namespace {
void validatePredicateValue(const Column &column,
                            const unidb::sql::SqlValue &value) {
  if (column.type == DataType::INT &&
      std::holds_alternative<std::int64_t>(value)) {
    return;
  }
  if (column.type == DataType::VARCHAR &&
      std::holds_alternative<std::string>(value)) {
    return;
  }
  throw std::invalid_argument("Predicate value type does not match column: " +
                              column.name);
}
} // namespace

TupleDeleteRowsAdapter::TupleDeleteRowsAdapter(
    ::GetCatalogRootPageIdAction &root, ::GetTableByNameAction &table,
    ::ScanTupleTableWithLocationsAction &scanAction,
    ::DeleteTupleAction &deleteAction)
    : getRoot(root), getTable(table), scan(scanAction),
      deleteTuple(deleteAction) {}
std::size_t
TupleDeleteRowsAdapter::erase(const std::string &tableName,
                              const unidb::sql::WhereEquals &condition) {
  constexpr CatalogPageId metadataPageId = 1;
  const auto root = getRoot.execute(metadataPageId);
  const auto table = getTable.execute(root, tableName);
  auto column = std::find_if(table.columns.begin(), table.columns.end(),
                             [&](const Column &candidate) {
                               return candidate.name == condition.columnName;
                             });
  if (column == table.columns.end())
    throw std::invalid_argument("Unknown delete predicate column: " +
                                condition.columnName);
  validatePredicateValue(*column, condition.value);
  const auto index = static_cast<std::size_t>(column - table.columns.begin());
  if (table.firstTuplePageId == std::numeric_limits<CatalogPageId>::max())
    return 0;
  std::vector<LocatedTuple> matches;
  for (const auto &record : scan.execute(table.firstTuplePageId)) {
    const auto values = RdbTupleCodec::decode(table.columns, record.data);
    if (values[index] == condition.value)
      matches.push_back(record);
  }
  for (const auto &record : matches)
    deleteTuple.execute(record.pageId, record.slotId);
  return matches.size();
}
} // namespace unidb::rdb