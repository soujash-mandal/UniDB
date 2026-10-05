#pragma once

#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

class GetCatalogRootPageIdAction {
public:
  GetCatalogRootPageIdAction(FetchPagePort &fetchPagePort,
                             UnpinPagePort &unpinPagePort);

  CatalogPageId execute(CatalogPageId pageId);

private:
  FetchPagePort &fetchPagePort;
  UnpinPagePort &unpinPagePort;
};
