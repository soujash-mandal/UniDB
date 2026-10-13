#include "SelectRowsAction.h"
namespace unidb::rdb {
std::vector<std::vector<unidb::sql::SqlValue>> SelectRowsAction::execute(const unidb::sql::SelectStatement &statement) {
  return port.select(statement);
}
}