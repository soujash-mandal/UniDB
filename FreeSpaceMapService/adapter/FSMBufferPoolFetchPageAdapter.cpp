#include "FSMBufferPoolFetchPageAdapter.h"

FSMBufferPoolFetchPageAdapter::FSMBufferPoolFetchPageAdapter(
    FetchPageAction &fetchPageAction)
    : fetchPageAction(fetchPageAction) {}

Page &FSMBufferPoolFetchPageAdapter::fetchPage(PageId pageId) {
  return fetchPageAction.execute(pageId);
}
