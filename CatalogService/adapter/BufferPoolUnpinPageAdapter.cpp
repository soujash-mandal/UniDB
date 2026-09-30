#include "BufferPoolUnpinPageAdapter.h"

BufferPoolUnpinPageAdapter::BufferPoolUnpinPageAdapter(
    UnpinPageAction &unpinPageAction)
    : unpinPageAction(unpinPageAction) {}

void BufferPoolUnpinPageAdapter::unpinPage(CatalogPageId pageId) {
  unpinPageAction.execute(pageId);
}
