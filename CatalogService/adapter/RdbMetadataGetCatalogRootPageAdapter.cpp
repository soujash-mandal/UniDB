#include "RdbMetadataGetCatalogRootPageAdapter.h"

RdbMetadataGetCatalogRootPageAdapter::RdbMetadataGetCatalogRootPageAdapter(
    GetCatalogRootPageIdAction &getCatalogRootPageIdAction,
    CatalogPageId rdbMetadataPageId)
    : getCatalogRootPageIdAction(getCatalogRootPageIdAction),
      rdbMetadataPageId(rdbMetadataPageId) {}

CatalogPageId RdbMetadataGetCatalogRootPageAdapter::getCatalogRootPageId() {
  return getCatalogRootPageIdAction.execute(rdbMetadataPageId);
}
