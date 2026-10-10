#include "CatalogBufferPoolUnpinPageAdapter.h"

CatalogBufferPoolUnpinPageAdapter::CatalogBufferPoolUnpinPageAdapter(
    UnpinPageAction &unpinPageAction)
    : unpinPageAction(unpinPageAction) {}

void CatalogBufferPoolUnpinPageAdapter::unpinPage(CatalogPageId pageId) {
  unpinPageAction.execute(pageId);
}
