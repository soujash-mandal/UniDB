#include "ReadPageAdapter.h"

#include <cstring>

ReadPageAdapter::ReadPageAdapter(ReadPageAction &readPageAction)
    : readPageAction(readPageAction) {}

BufferPoolPage ReadPageAdapter::readPage(BufferPoolPageId pageId) {
  DiskPage diskPage = readPageAction.execute(pageId);
  BufferPoolPage bufferPoolPage;
  std::memcpy(bufferPoolPage.data(), diskPage.data(),
              BufferPoolPage::PAGE_SIZE);
  return bufferPoolPage;
}
