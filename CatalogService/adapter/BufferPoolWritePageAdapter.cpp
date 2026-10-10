#include "BufferPoolWritePageAdapter.h"
#include <cstring>

CatalogBufferPoolWritePageAdapter::CatalogBufferPoolWritePageAdapter(
    WritePageAction &writePageAction)
    : writePageAction(writePageAction) {}

void CatalogBufferPoolWritePageAdapter::writePage(CatalogPageId pageId,
                                                  CatalogPage page) {
  BufferPoolPage bufferPage;

  std::memcpy(bufferPage.data(), page.data(), CatalogPage::PAGE_SIZE);

  writePageAction.execute(pageId, bufferPage);
}
