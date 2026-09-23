#include "BufferPoolNewPageAdapter.h"

BufferPoolNewPageAdapter::BufferPoolNewPageAdapter(NewPageAction &newPageAction)
    : newPageAction(newPageAction) {}

NewPageResult BufferPoolNewPageAdapter::newPage() {
  return newPageAction.execute();
}
