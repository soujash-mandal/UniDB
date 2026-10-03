#include "BufferPoolWritePageAdapter.h"

#include <cstring>

BufferPoolWritePageAdapter::BufferPoolWritePageAdapter(
    WritePageAction &writePageAction)
    : writePageAction(writePageAction) {}

void BufferPoolWritePageAdapter::writePage(FSMPageId pageId, FSMPage page) {
  BufferPoolPage bufferPoolPage;
  std::memcpy(bufferPoolPage.data(), page.data(), PAGE_SIZE);
  writePageAction.execute(pageId, bufferPoolPage);
}
