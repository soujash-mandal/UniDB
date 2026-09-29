#include "BufferPoolFetchPageAdapter.h"

#include <cstring>

BufferPoolFetchPageAdapter::BufferPoolFetchPageAdapter(
    FetchPageAction &fetchPageAction)
    : fetchPageAction(fetchPageAction) {}

TuplePage BufferPoolFetchPageAdapter::fetchPage(TuplePageId pageId) {
  BufferPoolPage bufferPage = fetchPageAction.execute(pageId);
  TuplePage tuplePage;
  std::memcpy(tuplePage.data(), bufferPage.data(), TuplePage::PAGE_SIZE);

  return tuplePage;
}
