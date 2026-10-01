#include "BufferPoolManagerReadRdbMetadataPageAdapter.h"

#include <cstring>

BufferPoolManagerReadRdbMetadataPageAdapter::
    BufferPoolManagerReadRdbMetadataPageAdapter(
        FetchPageAction &fetchPageAction)
    : fetchPageAction(fetchPageAction) {}

RdbMetadataPage
BufferPoolManagerReadRdbMetadataPageAdapter::readPage(CatalogPageId pageId) {
  BufferPoolPage bufferPoolPage = fetchPageAction.execute(pageId);
  RdbMetadataPage metadataPage;
  std::memcpy(metadataPage.data(), bufferPoolPage.data(),
              RdbMetadataPage::PAGE_SIZE);
  return metadataPage;
}
