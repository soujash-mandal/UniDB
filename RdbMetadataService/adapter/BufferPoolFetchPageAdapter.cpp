#include "BufferPoolFetchPageAdapter.h"

#include <cstring>

BufferPoolFetchPageAdapter::BufferPoolFetchPageAdapter(
    FetchPageAction &fetchPageAction)
    : fetchPageAction(fetchPageAction) {}

RdbMetadataPage BufferPoolFetchPageAdapter::fetchPage(CatalogPageId pageId) {

  BufferPoolPage bufferPage = fetchPageAction.execute(pageId);

  RdbMetadataPage metadataPage;

  std::memcpy(metadataPage.data(), bufferPage.data(),
              RdbMetadataPage::PAGE_SIZE);

  return metadataPage;
}
