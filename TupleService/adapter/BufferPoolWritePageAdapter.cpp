#include "BufferPoolWritePageAdapter.h"

#include <cstring>

TupleBufferPoolWritePageAdapter::TupleBufferPoolWritePageAdapter(
    WritePageAction &writePageAction)
    : writePageAction(writePageAction) {}

void TupleBufferPoolWritePageAdapter::writePage(TuplePageId pageId,
                                                TuplePage page) {
  BufferPoolPage bufferPoolPage;
  std::memcpy(bufferPoolPage.data(), page.data(), PAGE_SIZE);
  writePageAction.execute(pageId, bufferPoolPage);
}
