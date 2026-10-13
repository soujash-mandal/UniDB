#include "RdbServiceAdapter.h"

#include <utility>

namespace unidb::query {
namespace {
RdbServiceErrorCode translateErrorCode(unidb::rdb::RdbErrorCode code) {
  switch (code) {
  case unidb::rdb::RdbErrorCode::InvalidArgument:
    return RdbServiceErrorCode::InvalidArgument;
  case unidb::rdb::RdbErrorCode::TableNotFound:
    return RdbServiceErrorCode::TableNotFound;
  case unidb::rdb::RdbErrorCode::DuplicateTable:
    return RdbServiceErrorCode::DuplicateTable;
  case unidb::rdb::RdbErrorCode::UnknownColumn:
    return RdbServiceErrorCode::UnknownColumn;
  case unidb::rdb::RdbErrorCode::TypeMismatch:
    return RdbServiceErrorCode::TypeMismatch;
  case unidb::rdb::RdbErrorCode::UnsupportedType:
    return RdbServiceErrorCode::UnsupportedType;
  case unidb::rdb::RdbErrorCode::StorageFailure:
    return RdbServiceErrorCode::StorageFailure;
  case unidb::rdb::RdbErrorCode::InternalError:
    return RdbServiceErrorCode::InternalError;
  }
  return RdbServiceErrorCode::InternalError;
}

template <typename Out, typename In, typename Mapper>
RdbServiceResult<Out>
translateResult(const unidb::rdb::RdbResult<In> &result, Mapper mapper) {
  if (!result.hasValue()) {
    const auto &error = result.error();
    return RdbServiceResult<Out>::failure(
        {translateErrorCode(error.code), error.message});
  }
  return RdbServiceResult<Out>::success(mapper(result.value()));
}
} // namespace

RdbServiceAdapter::RdbServiceAdapter(
    unidb::rdb::CreateTableAction &createAction,
    unidb::rdb::DropTableAction &dropAction,
    unidb::rdb::GetTableDefinitionAction &getAction,
    unidb::rdb::InsertRowAction &insertAction,
    unidb::rdb::SelectRowsAction &selectAction,
    unidb::rdb::DeleteRowsAction &deleteAction)
    : create(createAction), drop(dropAction), getDefinition(getAction),
      insert(insertAction), select(selectAction), erase(deleteAction) {}

RdbServiceResult<uint32_t>
RdbServiceAdapter::createTable(const unidb::sql::CreateTableStatement &statement) {
  return translateResult<uint32_t>(create.execute(statement),
                                   [](uint32_t id) { return id; });
}

RdbServiceStatus RdbServiceAdapter::dropTable(const std::string &name) {
  return translateResult<std::monostate>(drop.execute(name),
                                         [](const std::monostate &value) {
                                           return value;
                                         });
}

RdbServiceResult<QueryTableDefinition>
RdbServiceAdapter::getTableDefinition(const std::string &name) {
  return translateResult<QueryTableDefinition>(
      getDefinition.execute(name), [](const unidb::rdb::TableDefinition &table) {
        return QueryTableDefinition{table.tableId, table.tableName, table.columns};
      });
}

RdbServiceResult<std::size_t>
RdbServiceAdapter::insertRow(const unidb::sql::InsertStatement &statement) {
  return translateResult<std::size_t>(
      insert.execute(statement), [](std::size_t count) { return count; });
}

RdbServiceResult<std::vector<std::vector<unidb::sql::SqlValue>>>
RdbServiceAdapter::selectRows(const unidb::sql::SelectStatement &statement) {
  return translateResult<std::vector<std::vector<unidb::sql::SqlValue>>>(
      select.execute(statement),
      [](const std::vector<std::vector<unidb::sql::SqlValue>> &rows) {
        return rows;
      });
}

RdbServiceResult<std::size_t> RdbServiceAdapter::deleteRows(
    const std::string &name, const unidb::sql::WhereEquals &condition) {
  return translateResult<std::size_t>(
      erase.execute(name, condition),
      [](std::size_t count) { return count; });
}
} // namespace unidb::query
