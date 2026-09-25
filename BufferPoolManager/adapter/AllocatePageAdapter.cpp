#include "AllocatePageAdapter.h"

AllocatePageAdapter::AllocatePageAdapter(AllocatePageAction &allocatePageAction)
    : allocatePageAction(allocatePageAction) {}

BufferPoolPageId AllocatePageAdapter::allocatePage() {
  BufferPoolPageId pageId = allocatePageAction.execute();
  return pageId;
}
