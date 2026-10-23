#include "TupleSelectRowsAdapter.h"
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

TupleSelectRowsAdapter::TupleSelectRowsAdapter(
    ::GetCatalogRootPageIdAction &root, ::GetTableByNameAction &table,
    ::ScanTupleTableAction &scanAction)
    : getRoot(root), getTable(table), scan(scanAction) {}
std::vector<std::vector<unidb::sql::SqlValue>>
TupleSelectRowsAdapter::select(const unidb::sql::SelectStatement &statement) {
  constexpr CatalogPageId metadataPageId = 1;
  const auto root = getRoot.execute(metadataPageId);
  const auto table = getTable.execute(root, statement.tableName);
  std::vector<std::size_t> projection;
  if (statement.selectAll) {
    for (std::size_t i = 0; i < table.columns.size(); ++i)
      projection.push_back(i);
  } else {
    for (const auto &name : statement.columnNames) {
      auto it = std::find_if(table.columns.begin(), table.columns.end(),
                             [&](const Column &c) { return c.name == name; });
      if (it == table.columns.end())
        throw std::invalid_argument("Unknown selected column: " + name);
      projection.push_back(
          static_cast<std::size_t>(it - table.columns.begin()));
    }
  }
  std::size_t predicateIndex = 0;
  const auto *predicate =
      std::get_if<unidb::sql::WhereEquals>(&statement.where);
  if (predicate) {
    auto it = std::find_if(
        table.columns.begin(), table.columns.end(),
        [&](const Column &c) { return c.name == predicate->columnName; });
    if (it == table.columns.end())
      throw std::invalid_argument("Unknown predicate column: " +
                                  predicate->columnName);
    validatePredicateValue(*it, predicate->value);
    predicateIndex = static_cast<std::size_t>(it - table.columns.begin());
  }
  if (table.firstTuplePageId == std::numeric_limits<CatalogPageId>::max())
    return {};
  std::vector<std::vector<unidb::sql::SqlValue>> rows;
  for (const auto &bytes : scan.execute(table.firstTuplePageId)) {
    auto values = RdbTupleCodec::decode(table.columns, bytes);
    if (predicate && values[predicateIndex] != predicate->value)
      continue;
    std::vector<unidb::sql::SqlValue> projected;
    projected.reserve(projection.size());
    for (const auto index : projection)
      projected.push_back(values[index]);
    rows.push_back(std::move(projected));
  }
  return rows;
}
} // namespace unidb::rdb
