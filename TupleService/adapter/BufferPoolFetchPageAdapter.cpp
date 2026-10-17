#include "BufferPoolFetchPageAdapter.h"

#include <cstring>

TupleBufferPoolFetchPageAdapter::TupleBufferPoolFetchPageAdapter(
    FetchPageAction &fetchPageAction, UnpinPageAction &unpinPageAction)
    : fetchPageAction(fetchPageAction), unpinPageAction(unpinPageAction) {}

TuplePage TupleBufferPoolFetchPageAdapter::fetchPage(TuplePageId pageId) {
  BufferPoolPage bufferPage = fetchPageAction.execute(pageId);
  TuplePage tuplePage;
  try {
    tuplePage.loadFromBytes(bufferPage.data());
  } catch (...) {
    try {
      unpinPageAction.execute(pageId);
    } catch (...) {
      // Preserve the original page validation failure.
    }
    throw;
  }
  return tuplePage;
}
