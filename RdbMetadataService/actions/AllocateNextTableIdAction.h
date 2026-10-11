#pragma once

#include "../domain/RdbMetadataPage.h"
#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class AllocateNextTableIdAction {
public:
  AllocateNextTableIdAction(RdbMetadataFetchPagePort &fetchPagePort,
                            RdbMetadataWritePagePort &writePagePort,
                            RdbMetadataUnpinPagePort &unpinPagePort);

  CatalogTableId execute(CatalogPageId pageId);

private:
  RdbMetadataFetchPagePort &fetchPagePort;
  RdbMetadataWritePagePort &writePagePort;
  RdbMetadataUnpinPagePort &unpinPagePort;
};
