#pragma once

#include "../domain/CatalogPage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

class GetTableAction {
public:
  GetTableAction(CatalogFetchPagePort &fetchPagePort, CatalogUnpinPagePort &unpinPagePort);

  Table execute(CatalogPageId rootPageId, CatalogTableId tableId);

private:
  CatalogFetchPagePort &fetchPagePort;
  CatalogUnpinPagePort &unpinPagePort;
};
