#include "FSMBufferPoolNewPageAdapter.h"

FSMBufferPoolNewPageAdapter::FSMBufferPoolNewPageAdapter(
    NewPageAction &newPageAction)
    : newPageAction(newPageAction) {}

NewPageResult FSMBufferPoolNewPageAdapter::newPage() {
  return newPageAction.execute();
}
