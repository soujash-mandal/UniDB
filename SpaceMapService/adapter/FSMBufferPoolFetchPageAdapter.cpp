#include "FSMBufferPoolFetchPageAdapter.h"

FSMBufferPoolFetchPageAdapter::FSMBufferPoolFetchPageAdapter(
    FetchPageAction &fetchPageAction)
    : fetchPageAction(fetchPageAction) {}

FSMPage FSMBufferPoolFetchPageAdapter::fetchPage(FSMPageId pageId) {
  BufferPoolPage bufferPage = fetchPageAction.execute(pageId);
  FSMPage fsmPage;
  std::memcpy(fsmPage.data(), bufferPage.data(), PAGE_SIZE);
  return fsmPage;
}
