#include "AllocatePageAdapter.h"

AllocatePageAdapter::AllocatePageAdapter(AllocatePageAction &allocatePageAction)
    : allocatePageAction(allocatePageAction) {}

PageId AllocatePageAdapter::allocatePage() {
  return allocatePageAction.execute();
}
