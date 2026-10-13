#include "GetTableDefinitionAction.h"
namespace unidb::rdb {
RdbResult<TableDefinition> GetTableDefinitionAction::execute(const std::string &tableName) {
  try { return RdbResult<TableDefinition>::success(port.get(tableName)); }
  catch (const std::exception &e) { return RdbResult<TableDefinition>::failure(classifyError(e)); }
}
}