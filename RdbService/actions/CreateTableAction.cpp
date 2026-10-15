#include "CreateTableAction.h"
namespace unidb::rdb {
RdbResult<uint32_t> CreateTableAction::execute(const unidb::sql::CreateTableStatement &statement) {
  try { return RdbResult<uint32_t>::success(port.create(statement)); }
  catch (const std::exception &e) { return RdbResult<uint32_t>::failure(classifyError(e)); }
}
}