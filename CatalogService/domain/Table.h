#pragma once

#include <string>
#include <vector>

#include "../../core/TableId.h"
#include "Column.h"

struct Table {
  TableId tableId;
  std::string name;
  std::vector<Column> columns;
};
