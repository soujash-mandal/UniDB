#pragma once

#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class UpdateRdbMetadataPageAction {
public:
  UpdateRdbMetadataPageAction(RdbMetadataFetchPagePort &fetchPagePort,
                              RdbMetadataWritePagePort &writePagePort,
                              RdbMetadataUnpinPagePort &unpinPagePort);

  void execute(CatalogPageId pageId, CatalogPageId catalogRootPageId);

private:
  RdbMetadataFetchPagePort &readPagePort;
  RdbMetadataWritePagePort &writePagePort;
  RdbMetadataUnpinPagePort &unpinPagePort;
};
