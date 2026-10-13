#include "DeleteRowsAction.h"
namespace unidb::rdb {
RdbResult<std::size_t> DeleteRowsAction::execute(const std::string &tableName, const unidb::sql::WhereEquals &condition) {
  try { return RdbResult<std::size_t>::success(port.erase(tableName, condition)); }
  catch (const std::exception &e) { return RdbResult<std::size_t>::failure(classifyError(e)); }
}
}