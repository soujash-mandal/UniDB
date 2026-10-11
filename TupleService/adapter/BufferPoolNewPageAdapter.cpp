#include "BufferPoolNewPageAdapter.h"

TupleBufferPoolNewPageAdapter::TupleBufferPoolNewPageAdapter(
    NewPageAction &newPageAction)
    : newPageAction(newPageAction) {}

TuplePageId TupleBufferPoolNewPageAdapter::newPage() {
  return newPageAction.execute();
}
