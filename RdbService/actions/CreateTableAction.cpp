#include "CreateTableAction.h"
#include <stdexcept>
namespace unidb::rdb {
CatalogTableId CreateTableAction::execute(const unidb::sql::CreateTableStatement &statement) {
  if (statement.tableName.empty() || statement.columns.empty())
    throw std::invalid_argument("A table needs a name and at least one column");
  std::vector<Column> columns;
  columns.reserve(statement.columns.size());
  for (const auto &column : statement.columns) {
    if (column.name.empty()) throw std::invalid_argument("Column name cannot be empty");
    DataType type;
    switch (column.type) {
      case unidb::sql::SqlType::Int: type = DataType::INT; break;
      case unidb::sql::SqlType::Text: type = DataType::VARCHAR; break;
      default: throw std::invalid_argument("Unsupported SQL column type");
    }
    columns.push_back(Column{column.name, type, 0, false});
  }
  constexpr CatalogPageId metadataPageId = 1;
  const auto root = getRoot.execute(metadataPageId);
  const auto id = allocateId.execute(metadataPageId);
  return createTable.execute(root, id, statement.tableName, std::move(columns));
}
}
