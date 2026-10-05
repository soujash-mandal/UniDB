#pragma once

#include "../port/FetchPagePort.h"
#include "../port/UnpinPagePort.h"
#include "../port/WritePagePort.h"

class UpdateRdbMetadataPageAction {
public:
  UpdateRdbMetadataPageAction(FetchPagePort &fetchPagePort,
                              WritePagePort &writePagePort,
                              UnpinPagePort &unpinPagePort);

  void execute(CatalogPageId pageId, CatalogPageId catalogRootPageId);

private:
  FetchPagePort &readPagePort;
  WritePagePort &writePagePort;
  UnpinPagePort &unpinPagePort;
};
