#include "RdbMetadataAdapter.h"

#include "../../RdbMetadataService/actions/AllocateNextTableIdAction.h"
#include "../../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"

RdbMetadataAdapter::RdbMetadataAdapter(
    GetCatalogRootPageIdAction &getCatalogRootPageIdAction,
    AllocateNextTableIdAction &allocateNextTableIdAction,
    MetadataPageId metadataPageId)
    : getCatalogRootPageIdAction(getCatalogRootPageIdAction),
      allocateNextTableIdAction(allocateNextTableIdAction),
      metadataPageId(metadataPageId) {}

CatalogPageId RdbMetadataAdapter::getCatalogRootPageId() {
  return getCatalogRootPageIdAction.execute(metadataPageId);
}

uint32_t RdbMetadataAdapter::allocateNextTableId() {
  return allocateNextTableIdAction.execute(metadataPageId);
}
