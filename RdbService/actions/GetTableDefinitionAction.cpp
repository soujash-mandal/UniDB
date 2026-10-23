#include "GetTableDefinitionAction.h"
namespace unidb::rdb {
TableDefinition
GetTableDefinitionAction::execute(const std::string &tableName) {
  return port.get(tableName);
}
} // namespace unidb::rdb