#include "CatalogBufferPoolNewPageAdapter.h"

CatalogBufferPoolNewPageAdapter::CatalogBufferPoolNewPageAdapter(NewPageAction &newPageAction)
    : newPageAction(newPageAction) {}

CatalogPageId CatalogBufferPoolNewPageAdapter::newPage() {
  return newPageAction.execute();
}
