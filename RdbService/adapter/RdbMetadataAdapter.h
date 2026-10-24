#pragma once

#include "../port/RdbMetadataPort.h"

class AllocateNextTableIdAction;
class GetCatalogRootPageIdAction;

class RdbMetadataAdapter final : public RdbMetadataPort {
public:
  RdbMetadataAdapter(GetCatalogRootPageIdAction &getCatalogRootPageIdAction,
                     AllocateNextTableIdAction &allocateNextTableIdAction,
                     MetadataPageId metadataPageId);

  CatalogPageId getCatalogRootPageId() override;
  uint32_t allocateNextTableId() override;

private:
  GetCatalogRootPageIdAction &getCatalogRootPageIdAction;
  AllocateNextTableIdAction &allocateNextTableIdAction;
  MetadataPageId metadataPageId;
};
