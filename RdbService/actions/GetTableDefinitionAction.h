#pragma once
#include "../port/GetTableDefinitionPort.h"
namespace unidb::rdb {
class GetTableDefinitionAction {
public:
  explicit GetTableDefinitionAction(GetTableDefinitionPort &port)
      : port(port) {}
  TableDefinition execute(const std::string &tableName);

private:
  GetTableDefinitionPort &port;
};
} // namespace unidb::rdb