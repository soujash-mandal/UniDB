#include "BufferPoolWritePageAdapter.h"

#include <cstring>

BufferPoolWritePageAdapter::BufferPoolWritePageAdapter(
    WritePageAction &writePageAction)
    : writePageAction(writePageAction) {}

void BufferPoolWritePageAdapter::writePage(CatalogPageId pageId,
                                           RdbMetadataPage page) {

  BufferPoolPage bufferPage;

  std::memcpy(bufferPage.data(), page.data(), RdbMetadataPage::PAGE_SIZE);

  writePageAction.execute(pageId, bufferPage);
}
