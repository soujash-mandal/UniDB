#include "GetCatalogRootPageIdAdapter.h"

GetCatalogRootPageIdAdapter::GetCatalogRootPageIdAdapter(
    GetCatalogRootPageIdAction &action)
    : action(action) {}

uint32_t GetCatalogRootPageIdAdapter::getCatalogRootPageId(
    uint32_t metadataPageId) {
  return action.execute(metadataPageId);
}
