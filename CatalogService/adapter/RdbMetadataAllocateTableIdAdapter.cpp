#include "RdbMetadataAllocateTableIdAdapter.h"

RdbMetadataAllocateTableIdAdapter::RdbMetadataAllocateTableIdAdapter(
    AllocateNextTableIdAction &allocateNextTableIdAction,
    CatalogPageId rdbMetadataPageId)
    : allocateNextTableIdAction(allocateNextTableIdAction),
      rdbMetadataPageId(rdbMetadataPageId) {}

CatalogTableId RdbMetadataAllocateTableIdAdapter::allocateTableId() {
  return allocateNextTableIdAction.execute(rdbMetadataPageId);
}
