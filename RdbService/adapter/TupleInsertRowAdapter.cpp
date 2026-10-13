#include "TupleInsertRowAdapter.h"
#include "RdbTupleCodec.h"
#include <limits>
#include <algorithm>
#include <stdexcept>
namespace unidb::rdb {
TupleInsertRowAdapter::TupleInsertRowAdapter(::GetCatalogRootPageIdAction &root, ::GetTableByNameAction &table, ::UpdateTableAction &update, ::InsertTupleIntoTableAction &insert) : getRoot(root), getTable(table), updateTable(update), insertTuple(insert) {}
void TupleInsertRowAdapter::insert(const unidb::sql::InsertStatement &statement) {
  constexpr CatalogPageId metadataPageId = 1;
  const auto root = getRoot.execute(metadataPageId);
  auto table = getTable.execute(root, statement.tableName);
  std::vector<unidb::sql::SqlValue> orderedValues = statement.values;
  if (!statement.columnNames.empty()) {
    if (statement.columnNames.size() != table.columns.size() || statement.values.size() != statement.columnNames.size())
      throw std::invalid_argument("V1 INSERT requires values for every table column");
    orderedValues.resize(table.columns.size());
    std::vector<bool> assigned(table.columns.size(), false);
    for (std::size_t i = 0; i < statement.columnNames.size(); ++i) {
      auto column = std::find_if(table.columns.begin(), table.columns.end(), [&](const Column &candidate) { return candidate.name == statement.columnNames[i]; });
      if (column == table.columns.end()) throw std::invalid_argument("Unknown INSERT column: " + statement.columnNames[i]);
      const auto index = static_cast<std::size_t>(column - table.columns.begin());
      if (assigned[index]) throw std::invalid_argument("Duplicate INSERT column: " + statement.columnNames[i]);
      assigned[index] = true;
      orderedValues[index] = statement.values[i];
    }
  }

  std::vector<unidb::sql::SqlType> schema;
  schema.reserve(table.columns.size());
  for (const auto &column : table.columns) {
    switch (column.type) {
    case DataType::INT: schema.push_back(unidb::sql::SqlType::Int); break;
    case DataType::VARCHAR: schema.push_back(unidb::sql::SqlType::Text); break;
    default: throw std::invalid_argument("Column type is not supported by the V1 SQL contract");
    }
  }

  auto bytes = RdbTupleCodec::encode(schema, orderedValues);
  const auto result = insertTuple.execute(table.firstTuplePageId, bytes);
  if (table.firstTuplePageId == std::numeric_limits<CatalogPageId>::max()) {
    table.firstTuplePageId = result.firstPageId;
    updateTable.execute(root, table);
  }
}
}
