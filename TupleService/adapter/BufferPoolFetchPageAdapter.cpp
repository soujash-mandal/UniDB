#include "BufferPoolFetchPageAdapter.h"

BufferPoolFetchPageAdapter::BufferPoolFetchPageAdapter(
    FetchPageAction &fetchPageAction)
    : fetchPageAction(fetchPageAction) {}

Page &BufferPoolFetchPageAdapter::fetchPage(PageId pageId) {
  return fetchPageAction.execute(pageId);
}
