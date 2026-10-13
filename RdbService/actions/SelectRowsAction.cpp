#include "SelectRowsAction.h"
namespace unidb::rdb {
RdbResult<std::vector<std::vector<unidb::sql::SqlValue>>> SelectRowsAction::execute(const unidb::sql::SelectStatement &statement) {
  try { return RdbResult<std::vector<std::vector<unidb::sql::SqlValue>>>::success(port.select(statement)); }
  catch (const std::exception &e) { return RdbResult<std::vector<std::vector<unidb::sql::SqlValue>>>::failure(classifyError(e)); }
}
}