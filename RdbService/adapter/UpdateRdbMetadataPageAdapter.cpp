#include "UpdateRdbMetadataPageAdapter.h"

UpdateRdbMetadataPageAdapter::UpdateRdbMetadataPageAdapter(
    UpdateRdbMetadataPageAction &updateRdbMetadataPageAction)
    : updateRdbMetadataPageAction(updateRdbMetadataPageAction) {}

void UpdateRdbMetadataPageAdapter::updateRdbMetadataPage(
    MetadataPageId pageId, CatalogPageId catalogRootPageId) {
  updateRdbMetadataPageAction.execute(pageId, catalogRootPageId);
}
