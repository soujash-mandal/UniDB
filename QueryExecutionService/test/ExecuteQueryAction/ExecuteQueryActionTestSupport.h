#pragma once

#include "../../actions/ExecuteQueryAction.h"
#include "../../domain/parser/SqlParserImpl.h"
#include "../../port/RdbServicePort.h"

#include <stdexcept>
#include <string>
#include <vector>

class FakeQesRdbServicePort final
    : public QueryExecutionService::RdbServicePort {
public:
  std::vector<unidb::sql::ColumnDefinition> schema{
      {"id", unidb::sql::SqlType::Int}, {"name", unidb::sql::SqlType::Text}};
  uint32_t createTableCalls = 0;
  uint32_t getSchemaCalls = 0;
  uint32_t insertCalls = 0;
  uint32_t selectCalls = 0;
  std::string tableName;
  std::vector<unidb::sql::ColumnDefinition> createdColumns;
  std::vector<std::vector<unidb::sql::SqlValue>> rows;
  bool fail = false;
  uint32_t failOnInsertCall = 0;
  uint32_t commitThenFailOnInsertCall = 0;
  std::vector<std::vector<unidb::sql::SqlValue>> insertedRows;

  uint32_t
  createTable(const unidb::sql::CreateTableStatement &statement) override {
    ++createTableCalls;
    tableName = statement.tableName;
    createdColumns = statement.columns;
    if (fail) {
      throw std::runtime_error("fake RDB failure");
    }
    return 7;
  }

  std::vector<unidb::sql::ColumnDefinition>
  getTableDefinition(const std::string &name) override {
    ++getSchemaCalls;
    tableName = name;
    if (fail) {
      throw std::runtime_error("fake RDB failure");
    }
    return schema;
  }

  void insertRow(const std::string &name,
                 const std::vector<unidb::sql::SqlValue> &values) override {
    ++insertCalls;
    tableName = name;
    insertedRows.push_back(values);
    if (commitThenFailOnInsertCall != 0 &&
        insertCalls == commitThenFailOnInsertCall) {
      throw QueryExecutionService::RowInsertedButPostInsertUpdateFailed(
          "fake post-insert bookkeeping failure");
    }
    if (fail || (failOnInsertCall != 0 && insertCalls == failOnInsertCall)) {
      throw std::runtime_error("fake RDB failure");
    }
  }

  std::vector<std::vector<unidb::sql::SqlValue>>
  selectAllRows(const std::string &name) override {
    ++selectCalls;
    tableName = name;
    if (fail) {
      throw std::runtime_error("fake RDB failure");
    }
    return rows;
  }
};

inline QueryExecutionService::ExecuteQueryAction
makeQesAction(FakeQesRdbServicePort &port, unidb::sql::SqlParserImpl &parser) {
  return QueryExecutionService::ExecuteQueryAction(port, parser);
}
