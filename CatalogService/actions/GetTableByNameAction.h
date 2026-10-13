#pragma once

#include "../domain/CatalogTypes.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

#include <string>

class GetTableByNameAction {
public:
  GetTableByNameAction(CatalogFetchPagePort &fetchPagePort,
                       CatalogUnpinPagePort &unpinPagePort);

  CatalogTableDefinition execute(CatalogPageId rootPageId,
                                 const std::string &tableName);

private:
  CatalogFetchPagePort &fetchPagePort;
  CatalogUnpinPagePort &unpinPagePort;
};
