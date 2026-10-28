#include "DeallocatePageAdapter.h"

DeallocatePageAdapter::DeallocatePageAdapter(DeallocatePageAction &action)
    : action(action) {}

void DeallocatePageAdapter::deallocatePage(BufferPoolPageId pageId) {
  action.execute(pageId);
}
