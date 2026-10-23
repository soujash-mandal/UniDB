#include "DeleteRowsAction.h"
namespace unidb::rdb {
std::size_t
DeleteRowsAction::execute(const std::string &tableName,
                          const unidb::sql::WhereEquals &condition) {
  return port.erase(tableName, condition);
}
} // namespace unidb::rdb