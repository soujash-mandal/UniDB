#include "BufferPoolUnpinPageAdapter.h"

BufferPoolUnpinPageAdapter::BufferPoolUnpinPageAdapter(
    UnpinPageAction &unpinPageAction)
    : unpinPageAction(unpinPageAction) {}

void BufferPoolUnpinPageAdapter::unpinPage(PageId pageId, bool dirty) {
  unpinPageAction.execute(pageId, dirty);
}
