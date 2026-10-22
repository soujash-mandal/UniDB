#include "BufferPoolWritePageAdapter.h"

#include "../../BufferPoolManager/domain/BufferPoolPage.h"

#include <cstring>

static_assert(TUPLE_PAGE_SIZE <= BufferPoolPage::PAGE_SIZE,
              "Tuple page must fit in a buffer-pool page");

TupleBufferPoolWritePageAdapter::TupleBufferPoolWritePageAdapter(
    WritePageAction &writePageAction)
    : writePageAction(writePageAction) {}

void TupleBufferPoolWritePageAdapter::writePage(TuplePageId pageId,
                                                TuplePage page) {
  BufferPoolPage bufferPoolPage;
  std::memcpy(bufferPoolPage.data(), page.data(), TUPLE_PAGE_SIZE);
  writePageAction.execute(pageId, bufferPoolPage);
}
