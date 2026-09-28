#include "WritePageAdapter.h"

#include <cstring>

WritePageAdapter::WritePageAdapter(WriteDiskPageAction &writePageAction)
    : writePageAction(writePageAction) {}

void WritePageAdapter::writePage(BufferPoolPageId pageId, BufferPoolPage page) {
  DiskPage diskPage;
  std::memcpy(diskPage.data(), page.data(), BufferPoolPage::PAGE_SIZE);
  writePageAction.execute(pageId, diskPage);
}
