#pragma once

#include "../domain/CatalogTypes.h"

#include <string>

class CatalogFetchPagePort;
class CatalogUnpinPagePort;

class GetCatalogTableDefinitionAction {
public:
  GetCatalogTableDefinitionAction(CatalogFetchPagePort &fetchPagePort,
                                  CatalogUnpinPagePort &unpinPagePort);
  CatalogTableDefinition execute(CatalogPageId rootPageId,
                                 const std::string &tableName);

private:
  CatalogFetchPagePort &fetchPagePort;
  CatalogUnpinPagePort &unpinPagePort;
};
