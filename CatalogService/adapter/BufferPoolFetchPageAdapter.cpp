#include "BufferPoolFetchPageAdapter.h"
#include <cstring>

BufferPoolFetchPageAdapter::BufferPoolFetchPageAdapter(
    FetchPageAction &fetchPageAction)
    : fetchPageAction(fetchPageAction) {}

CatalogPage BufferPoolFetchPageAdapter::fetchPage(CatalogPageId pageId) {
  BufferPoolPage bufferPage = fetchPageAction.execute(pageId);
  CatalogPage catalogPage;
  std::memcpy(catalogPage.data(), bufferPage.data(), CatalogPage::PAGE_SIZE);
  return catalogPage;
}
