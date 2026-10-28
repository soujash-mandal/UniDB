#include "BufferPoolWritePageAdapter.h"

#include <cstring>

SpaceMapBufferPoolWritePageAdapter::SpaceMapBufferPoolWritePageAdapter(
    WritePageAction &writePageAction)
    : writePageAction(writePageAction) {}

void SpaceMapBufferPoolWritePageAdapter::writePage(SpaceMapPageId pageId,
                                                   SpaceMapPage page) {
  BufferPoolPage bufferPoolPage;
  std::memcpy(bufferPoolPage.data(), page.data(), SPACE_MAP_PAGE_SIZE);
  writePageAction.execute(pageId, bufferPoolPage);
}
