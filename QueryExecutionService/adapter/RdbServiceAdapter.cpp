#include "RdbServiceAdapter.h"
namespace unidb::query {
RdbServiceAdapter::RdbServiceAdapter(unidb::rdb::CreateTableAction &createAction, unidb::rdb::DropTableAction &dropAction, unidb::rdb::GetTableDefinitionAction &getAction, unidb::rdb::InsertRowAction &insertAction, unidb::rdb::SelectRowsAction &selectAction, unidb::rdb::DeleteRowsAction &deleteAction)
  : create(createAction), drop(dropAction), getDefinition(getAction), insert(insertAction), select(selectAction), erase(deleteAction) {}
unidb::rdb::RdbResult<uint32_t> RdbServiceAdapter::createTable(const unidb::sql::CreateTableStatement &s) { return create.execute(s); }
unidb::rdb::RdbStatus RdbServiceAdapter::dropTable(const std::string &name) { return drop.execute(name); }
unidb::rdb::RdbResult<unidb::rdb::TableDefinition> RdbServiceAdapter::getTableDefinition(const std::string &name) { return getDefinition.execute(name); }
unidb::rdb::RdbResult<std::size_t> RdbServiceAdapter::insertRow(const unidb::sql::InsertStatement &s) { return insert.execute(s); }
unidb::rdb::RdbResult<std::vector<std::vector<unidb::sql::SqlValue>>> RdbServiceAdapter::selectRows(const unidb::sql::SelectStatement &s) { return select.execute(s); }
unidb::rdb::RdbResult<std::size_t> RdbServiceAdapter::deleteRows(const std::string &name, const unidb::sql::WhereEquals &condition) { return erase.execute(name, condition); }
}