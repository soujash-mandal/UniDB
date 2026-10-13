#include "InsertRowAction.h"
namespace unidb::rdb {
RdbResult<std::size_t> InsertRowAction::execute(const unidb::sql::InsertStatement &statement) {
  try { port.insert(statement); return RdbResult<std::size_t>::success(1); }
  catch (const std::exception &e) { return RdbResult<std::size_t>::failure(classifyError(e)); }
}
}