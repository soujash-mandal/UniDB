#pragma once
#include "../../core/TableId.h"
#include <string>

class CatalogPort {
public:
  virtual ~CatalogPort() = default;
  virtual void createTable(TableId tableId, const std::string &tableName) = 0;
};
