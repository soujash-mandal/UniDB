#include "BufferPoolUnpinPageAdapter.h"

BufferPoolUnpinPageAdapter::BufferPoolUnpinPageAdapter(
    UnpinPageAction &unpinPageAction)
    : unpinPageAction(unpinPageAction) {}

void BufferPoolUnpinPageAdapter::unpinPage(TuplePageId pageId) {
  unpinPageAction.execute(pageId);
}
