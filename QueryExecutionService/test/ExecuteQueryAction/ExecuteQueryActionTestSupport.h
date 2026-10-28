#pragma once

#include "../../../SQLParser/SqlParserImpl.h"
#include "../../actions/ExecuteQueryAction.h"
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
  std::vector<char> insertedBytes;
  std::vector<std::vector<char>> rows;
  bool fail = false;

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
                 const std::vector<char> &bytes) override {
    ++insertCalls;
    tableName = name;
    insertedBytes = bytes;
    if (fail) {
      throw std::runtime_error("fake RDB failure");
    }
  }

  std::vector<std::vector<char>>
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
