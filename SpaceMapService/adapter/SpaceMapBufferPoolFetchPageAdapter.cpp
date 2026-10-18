#include "SpaceMapBufferPoolFetchPageAdapter.h"

SpaceMapBufferPoolFetchPageAdapter::SpaceMapBufferPoolFetchPageAdapter(
    FetchPageAction &fetchPageAction)
    : fetchPageAction(fetchPageAction) {}

SpaceMapPage SpaceMapBufferPoolFetchPageAdapter::fetchPage(SpaceMapPageId pageId) {
  BufferPoolPage bufferPage = fetchPageAction.execute(pageId);
  SpaceMapPage fsmPage;
  std::memcpy(fsmPage.data(), bufferPage.data(), PAGE_SIZE);
  return fsmPage;
}
