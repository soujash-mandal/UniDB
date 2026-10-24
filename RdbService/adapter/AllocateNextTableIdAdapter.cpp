#include "AllocateNextTableIdAdapter.h"

AllocateNextTableIdAdapter::AllocateNextTableIdAdapter(
    AllocateNextTableIdAction &action)
    : action(action) {}

uint32_t
AllocateNextTableIdAdapter::allocateNextTableId(uint32_t metadataPageId) {
  return action.execute(metadataPageId);
}
