#pragma once
#include "../domain/TableDefinition.h"
#include <string>
namespace unidb::rdb {
class GetTableDefinitionPort {
public:
  virtual ~GetTableDefinitionPort() = default;
  virtual TableDefinition get(const std::string &tableName) = 0;
};
}