#include "BufferPoolWritePageAdapter.h"
#include <cstring>

BufferPoolWritePageAdapter::BufferPoolWritePageAdapter(
    WritePageAction &writePageAction)
    : writePageAction(writePageAction) {}

void BufferPoolWritePageAdapter::writePage(CatalogPageId pageId,
                                           CatalogPage page) {

  BufferPoolPage bufferPage;

  std::memcpy(bufferPage.data(), page.data(), CatalogPage::PAGE_SIZE);

  writePageAction.execute(pageId, bufferPage);
}
