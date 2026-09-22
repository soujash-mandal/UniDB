#pragma once

#include <string>
#include <vector>

#include "../../core/PageId.h"
#include "../../core/TableId.h"
#include "Column.h"

struct Table {
  TableId tableId;
  std::string name;
  std::vector<Column> columns;
  PageId firstFreeSpaceMapPageId;
};
