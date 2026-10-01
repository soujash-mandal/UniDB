#pragma once

#include "../domain/RdbMetadataPage.h"
#include "../port/WriteRdbMetadataPagePort.h"

class InitializeRdbMetadataPageAction {
public:
  explicit InitializeRdbMetadataPageAction(
      WriteRdbMetadataPagePort &writeMetadataPagePort);
  void execute(CatalogPageId pageId);

private:
  WriteRdbMetadataPagePort &writeMetadataPagePort;
};
