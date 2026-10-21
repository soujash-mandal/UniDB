#pragma once

#include "../domain/CatalogPage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

#include <string>

class GetTableByNameAction {
public:
  GetTableByNameAction(CatalogFetchPagePort &fetchPagePort,
                       CatalogUnpinPagePort &unpinPagePort);

  Table execute(CatalogPageId rootPageId, const std::string &tableName);

private:
  CatalogFetchPagePort &fetchPagePort;
  CatalogUnpinPagePort &unpinPagePort;
};
