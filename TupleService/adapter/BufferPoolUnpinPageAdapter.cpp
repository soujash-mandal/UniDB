#include "BufferPoolUnpinPageAdapter.h"

TupleBufferPoolUnpinPageAdapter::TupleBufferPoolUnpinPageAdapter(
    UnpinPageAction &unpinPageAction)
    : unpinPageAction(unpinPageAction) {}

void TupleBufferPoolUnpinPageAdapter::unpinPage(TuplePageId pageId) {
  unpinPageAction.execute(pageId);
}
