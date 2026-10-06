#include "CreateRdbMetadataPageAdapter.h"

CreateRdbMetadataPageAdapter::CreateRdbMetadataPageAdapter(
    CreateRdbMetadataPageAction &createRdbMetadataPageAction)
    : createRdbMetadataPageAction(createRdbMetadataPageAction) {}

CatalogPageId CreateRdbMetadataPageAdapter::createRdbMetadataPage() {
  return createRdbMetadataPageAction.execute();
}
