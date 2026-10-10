#include "CreateRdbMetadataPageAdapter.h"

CreateRdbMetadataPageAdapter::CreateRdbMetadataPageAdapter(
    CreateRdbMetadataPageAction &createRdbMetadataPageAction)
    : createRdbMetadataPageAction(createRdbMetadataPageAction) {}

MetadataPageId CreateRdbMetadataPageAdapter::createRdbMetadataPage() {
  return createRdbMetadataPageAction.execute();
}
