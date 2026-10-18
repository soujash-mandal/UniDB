#include "BufferPoolWritePageAdapter.h"

#include <cstring>

BufferPoolWritePageAdapter::BufferPoolWritePageAdapter(
    WritePageAction &writePageAction)
    : writePageAction(writePageAction) {}

void BufferPoolWritePageAdapter::writePage(SpaceMapPageId pageId,
                                           SpaceMapPage page) {
  BufferPoolPage bufferPoolPage;
  std::memcpy(bufferPoolPage.data(), page.data(), PAGE_SIZE);
  writePageAction.execute(pageId, bufferPoolPage);
}
