#include "TupleSelectRowsAdapter.h"
#include "RdbTupleCodec.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace unidb::rdb {
TupleSelectRowsAdapter::TupleSelectRowsAdapter(::GetCatalogRootPageIdAction &root, ::GetCatalogTableDefinitionAction &table, ::ScanTupleTableAction &scanAction) : getRoot(root), getTable(table), scan(scanAction) {}
std::vector<std::vector<unidb::sql::SqlValue>> TupleSelectRowsAdapter::select(const unidb::sql::SelectStatement &statement) {
  constexpr CatalogPageId metadataPageId = 1;
  const auto root = getRoot.execute(metadataPageId);
  const auto table = getTable.execute(root, statement.tableName);
  std::vector<std::size_t> projection;
  if (statement.selectAll) {
    for (std::size_t i = 0; i < table.columns.size(); ++i) projection.push_back(i);
  } else {
    for (const auto &name : statement.columnNames) {
      auto it = std::find_if(table.columns.begin(), table.columns.end(), [&](const Column &c) { return c.name == name; });
      if (it == table.columns.end()) throw std::runtime_error("Unknown selected column: " + name);
      projection.push_back(static_cast<std::size_t>(it - table.columns.begin()));
    }
  }
  std::size_t predicateIndex = 0;
  const auto *predicate = std::get_if<unidb::sql::WhereEquals>(&statement.where);
  if (predicate) {
    auto it = std::find_if(table.columns.begin(), table.columns.end(), [&](const Column &c) { return c.name == predicate->columnName; });
    if (it == table.columns.end()) throw std::runtime_error("Unknown predicate column: " + predicate->columnName);
    predicateIndex = static_cast<std::size_t>(it - table.columns.begin());
    const bool intColumn = it->type == DataType::INT;
    if ((intColumn && !std::holds_alternative<std::int64_t>(predicate->value)) ||
        (!intColumn && it->type == DataType::VARCHAR && !std::holds_alternative<std::string>(predicate->value))) {
      throw std::invalid_argument("Predicate value type does not match column type");
    }
  }
  if (table.tupleRootPageId == CATALOG_INVALID_PAGE_ID) return {};

  std::vector<unidb::sql::SqlType> schema;
  schema.reserve(table.columns.size());
  for (const auto &column : table.columns) {
    switch (column.type) {
    case DataType::INT: schema.push_back(unidb::sql::SqlType::Int); break;
    case DataType::VARCHAR: schema.push_back(unidb::sql::SqlType::Text); break;
    default: throw std::invalid_argument("Column type is not supported by the V1 SQL contract");
    }
  }

  std::vector<std::vector<unidb::sql::SqlValue>> rows;
  for (const auto &bytes : scan.execute(table.firstTuplePageId)) {
    auto values = RdbTupleCodec::decode(schema, bytes);
    if (predicate && values[predicateIndex] != predicate->value) continue;
    std::vector<unidb::sql::SqlValue> projected;
    projected.reserve(projection.size());
    for (const auto index : projection) projected.push_back(values[index]);
    rows.push_back(std::move(projected));
  }
  return rows;
}
}
