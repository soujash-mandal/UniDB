#include "BufferPoolFetchPageAdapter.h"

#include <cstring>

TupleBufferPoolFetchPageAdapter::TupleBufferPoolFetchPageAdapter(
    FetchPageAction &fetchPageAction)
    : fetchPageAction(fetchPageAction) {}

TuplePage TupleBufferPoolFetchPageAdapter::fetchPage(TuplePageId pageId) {
  BufferPoolPage bufferPage = fetchPageAction.execute(pageId);
  TuplePage tuplePage;
  std::memcpy(tuplePage.data(), bufferPage.data(), TUPLE_PAGE_SIZE);
  return tuplePage;
}
