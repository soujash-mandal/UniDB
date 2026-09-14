#pragma once
#include "../../core/TableId.h"
#include "../port/CatalogPort.h"
#include <string>

class CreateTableAction {
public:
  explicit CreateTableAction(CatalogPort &catalogPort);
  void execute(TableId tableId, const std::string &tableName);

private:
  CatalogPort &catalogPort;
};
