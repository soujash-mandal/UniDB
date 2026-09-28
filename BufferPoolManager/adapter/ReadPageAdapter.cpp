#include "ReadPageAdapter.h"

#include <cstring>

ReadPageAdapter::ReadPageAdapter(ReadDiskPageAction &readDiskPageAction)
    : readDiskPageAction(readDiskPageAction) {}

BufferPoolPage ReadPageAdapter::readDiskPage(BufferPoolPageId pageId) {
  DiskPage diskPage = readDiskPageAction.execute(pageId);
  BufferPoolPage bufferPoolPage;
  std::memcpy(bufferPoolPage.data(), diskPage.data(),
              BufferPoolPage::PAGE_SIZE);
  return bufferPoolPage;
}
