#include "ReclaimPageAdapter.h"

#include "../../BufferPoolManager/actions/DeletePageAction.h"

namespace RdbService {

ReclaimPageAdapter::ReclaimPageAdapter(DeletePageAction &deletePageAction)
    : deletePageAction(deletePageAction) {}

void ReclaimPageAdapter::reclaimPage(uint32_t pageId) {
  deletePageAction.execute(pageId);
}

} // namespace RdbService
