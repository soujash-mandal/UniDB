#include "DropTableAction.h"
namespace unidb::rdb {
RdbStatus DropTableAction::execute(const std::string &tableName) {
  try { port.drop(tableName); return RdbStatus::success(std::monostate{}); }
  catch (const std::exception &e) { return RdbStatus::failure(classifyError(e)); }
}
}