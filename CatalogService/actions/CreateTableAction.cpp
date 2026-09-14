#include "CreateTableAction.h"

CreateTableAction::CreateTableAction(CatalogPort &catalogPort)
    : catalogPort(catalogPort) {}

void CreateTableAction::execute(TableId tableId, const std::string &tableName) {
  catalogPort.createTable(tableId, tableName);
}
