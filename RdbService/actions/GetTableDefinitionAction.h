#pragma once
#include "../port/GetTableDefinitionPort.h"
#include "../domain/RdbResult.h"
namespace unidb::rdb {
class GetTableDefinitionAction {
public:
  explicit GetTableDefinitionAction(GetTableDefinitionPort &port) : port(port) {}
  RdbResult<TableDefinition> execute(const std::string &tableName);
private:
  GetTableDefinitionPort &port;
};
}