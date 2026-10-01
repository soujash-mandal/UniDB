#pragma once

#include "../domain/RdbMetadataPage.h"
#include "../port/ReadRdbMetadataPagePort.h"

class GetCatalogRootPageIdAction {
public:
  explicit GetCatalogRootPageIdAction(
      ReadRdbMetadataPagePort &readMetadataPagePort);
  CatalogPageId execute(CatalogPageId pageId);

private:
  ReadRdbMetadataPagePort &readMetadataPagePort;
};
