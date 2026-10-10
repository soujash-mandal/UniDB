#pragma once

#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"

class GetCatalogRootPageIdAction {
public:
  GetCatalogRootPageIdAction(RdbMetadataFetchPagePort &fetchPagePort,
                             RdbMetadataUnpinPagePort &unpinPagePort);

  CatalogPageId execute(CatalogPageId metadataPageId);

private:
  RdbMetadataFetchPagePort &fetchPagePort;
  RdbMetadataUnpinPagePort &unpinPagePort;
};
