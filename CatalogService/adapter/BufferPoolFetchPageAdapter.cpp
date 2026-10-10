#include "BufferPoolFetchPageAdapter.h"
#include <cstring>

CatalogBufferPoolFetchPageAdapter::CatalogBufferPoolFetchPageAdapter(
    FetchPageAction &fetchPageAction)
    : fetchPageAction(fetchPageAction) {}

CatalogPage CatalogBufferPoolFetchPageAdapter::fetchPage(
    CatalogPageId pageId) {
  BufferPoolPage bufferPage = fetchPageAction.execute(pageId);
  CatalogPage catalogPage;
  std::memcpy(catalogPage.data(), bufferPage.data(), CatalogPage::PAGE_SIZE);
  return catalogPage;
}
