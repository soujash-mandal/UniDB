#include "TupleInsertRowAdapter.h"
#include "../../QueryExecutionService/adapter/SqlTupleCodec.h"
#include <limits>
#include <stdexcept>
namespace unidb::rdb {
TupleInsertRowAdapter::TupleInsertRowAdapter(
    ::GetCatalogRootPageIdAction &root, ::GetTableByNameAction &table,
    ::UpdateTableAction &update, ::InsertTupleIntoTableAction &insert)
    : getRoot(root), getTable(table), updateTable(update), insertTuple(insert) {
}
void TupleInsertRowAdapter::insert(
    const unidb::sql::InsertStatement &statement) {
  constexpr CatalogPageId metadataPageId = 1;
  const auto root = getRoot.execute(metadataPageId);
  auto table = getTable.execute(root, statement.tableName);
  if (!statement.columnNames.empty()) {
    if (statement.columnNames.size() != table.columns.size())
      throw std::invalid_argument(
          "V1 INSERT requires values for every table column");
    for (std::size_t i = 0; i < table.columns.size(); ++i)
      if (statement.columnNames[i] != table.columns[i].name)
        throw std::invalid_argument(
            "V1 INSERT columns must follow table schema order");
  }
  auto bytes = SqlTupleCodec::encode(table.columns, statement.values);
  const auto result = insertTuple.execute(table.firstTuplePageId, bytes);
  if (table.firstTuplePageId == std::numeric_limits<CatalogPageId>::max()) {
    table.firstTuplePageId = result.firstPageId;
    updateTable.execute(root, table);
  }
}
} // namespace unidb::rdb
