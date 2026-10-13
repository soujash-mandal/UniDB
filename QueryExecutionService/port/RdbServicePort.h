#pragma once
#include "../../RdbService/domain/RdbResult.h"
#include "../../RdbService/port/GetTableDefinitionPort.h"
#include "../../SQLParserContract/sqlStatement.h"
#include <cstddef>
#include <string>
#include <vector>
namespace unidb::query {
class RdbServicePort {
public:
  virtual ~RdbServicePort() = default;
  virtual unidb::rdb::RdbResult<uint32_t> createTable(const unidb::sql::CreateTableStatement &) = 0;
  virtual unidb::rdb::RdbStatus dropTable(const std::string &) = 0;
  virtual unidb::rdb::RdbResult<unidb::rdb::TableDefinition> getTableDefinition(const std::string &) = 0;
  virtual unidb::rdb::RdbResult<std::size_t> insertRow(const unidb::sql::InsertStatement &) = 0;
  virtual unidb::rdb::RdbResult<std::vector<std::vector<unidb::sql::SqlValue>>> selectRows(const unidb::sql::SelectStatement &) = 0;
  virtual unidb::rdb::RdbResult<std::size_t> deleteRows(const std::string &, const unidb::sql::WhereEquals &) = 0;
};
}