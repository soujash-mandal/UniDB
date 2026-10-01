#include "BufferPoolManagerWriteRdbMetadataPageAdapter.h"

#include <cstring>

BufferPoolManagerWriteRdbMetadataPageAdapter::
    BufferPoolManagerWriteRdbMetadataPageAdapter(
        WritePageAction &writePageAction)
    : writePageAction(writePageAction) {}

void BufferPoolManagerWriteRdbMetadataPageAdapter::writePage(
    CatalogPageId pageId, RdbMetadataPage page) {
  BufferPoolPage bufferPoolPage;
  std::memcpy(bufferPoolPage.data(), page.data(), RdbMetadataPage::PAGE_SIZE);
  writePageAction.execute(pageId, bufferPoolPage);
}
