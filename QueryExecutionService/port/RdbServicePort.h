#pragma once

#include "../../SQLParserContract/sqlStatement.h"

#include <cstdint>
#include <string>
#include <vector>

namespace QueryExecutionService {

class RdbServicePort {
public:
  virtual ~RdbServicePort() = default;

  virtual uint32_t
  createTable(const unidb::sql::CreateTableStatement &statement) = 0;
  virtual std::vector<unidb::sql::ColumnDefinition>
  getTableDefinition(const std::string &tableName) = 0;
  virtual void insertRow(const std::string &tableName,
                         const std::vector<char> &tupleBytes) = 0;
  virtual std::vector<std::vector<char>>
  selectAllRows(const std::string &tableName) = 0;
};

} // namespace QueryExecutionService
