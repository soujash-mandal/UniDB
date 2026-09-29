#include "BufferPoolNewPageAdapter.h"

BufferPoolNewPageAdapter::BufferPoolNewPageAdapter(NewPageAction &newPageAction)
    : newPageAction(newPageAction) {}

TuplePageId BufferPoolNewPageAdapter::newPage() {
  return newPageAction.execute();
}
