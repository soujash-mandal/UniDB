#pragma once

#include "../port/CatalogPort.h"

#include <string>
#include <unordered_map>

class InMemoryCatalogAdapter : public CatalogPort {

public:
  void createTable(TableId tableId, const std::string &tableName) override;

private:
  TableId nextTableId = 1;

  std::unordered_map<std::string, TableId> tables;
};
