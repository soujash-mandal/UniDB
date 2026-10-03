#include "FSMBufferPoolNewPageAdapter.h"

FSMBufferPoolNewPageAdapter::FSMBufferPoolNewPageAdapter(
    NewPageAction &newPageAction)
    : newPageAction(newPageAction) {}

FSMPageId FSMBufferPoolNewPageAdapter::newPage() {
  return newPageAction.execute();
}
