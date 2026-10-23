#include "DropTableAction.h"
namespace unidb::rdb {
void DropTableAction::execute(const std::string &tableName) {
  port.drop(tableName);
}
} // namespace unidb::rdb