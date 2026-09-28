#include "BufferPoolWritePageAdapter.h"

BufferPoolWritePageAdapter::BufferPoolWritePageAdapter(
    WritePageAction &writePageAction)
    : writePageAction(writePageAction) {}

void BufferPoolWritePageAdapter::writePage(PageId pageId, Page page) {
  writePageAction.execute(pageId, page);
}
