#pragma once
#include "../../SQLParserContract/sqlStatement.h"
#include <cstdint>
#include <string>
#include <vector>
namespace unidb::rdb {
struct TableDefinition {
  uint32_t tableId{};
  std::string tableName;
  std::vector<unidb::sql::ColumnDefinition> columns;
};
class GetTableDefinitionPort {
public:
  virtual ~GetTableDefinitionPort() = default;
  virtual TableDefinition get(const std::string &tableName) = 0;
};
}