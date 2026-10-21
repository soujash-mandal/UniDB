#pragma once

#include "../domain/CatalogPage.h"
#include "../port/FetchPagePort.h"
#include "../port/NewPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class UpdateTableAction {
public:
  UpdateTableAction(CatalogFetchPagePort &fetchPagePort,
                    CatalogNewPagePort &newPagePort,
                    CatalogWritePagePort &writePagePort,
                    CatalogUnpinPagePort &unpinPagePort);

  void execute(CatalogPageId rootPageId, Table table);

private:
  CatalogFetchPagePort &fetchPagePort;
  CatalogNewPagePort &newPagePort;
  CatalogWritePagePort &writePagePort;
  CatalogUnpinPagePort &unpinPagePort;
};
