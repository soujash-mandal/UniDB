#include "BufferPoolNewPageAdapter.h"

BufferPoolNewPageAdapter::BufferPoolNewPageAdapter(NewPageAction &newPageAction)
    : newPageAction(newPageAction) {}

CatalogPageId BufferPoolNewPageAdapter::newPage() {
  return newPageAction.execute();
}
