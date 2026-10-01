#pragma once

#include "../domain/RdbMetadataPage.h"
#include "../port/ReadRdbMetadataPagePort.h"
#include "../port/WriteRdbMetadataPagePort.h"

class SetCatalogRootPageIdAction {
public:
  SetCatalogRootPageIdAction(ReadRdbMetadataPagePort &readMetadataPagePort,
                             WriteRdbMetadataPagePort &writeMetadataPagePort);

  void execute(CatalogPageId metadataPageId, CatalogPageId catalogRootPageId);

private:
  ReadRdbMetadataPagePort &readMetadataPagePort;
  WriteRdbMetadataPagePort &writeMetadataPagePort;
};
