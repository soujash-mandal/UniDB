#include "SpaceMapBufferPoolFetchPageAdapter.h"

#include <cstring>

SpaceMapBufferPoolFetchPageAdapter::SpaceMapBufferPoolFetchPageAdapter(
    FetchPageAction &fetchPageAction)
    : fetchPageAction(fetchPageAction) {}

SpaceMapPage
SpaceMapBufferPoolFetchPageAdapter::fetchPage(SpaceMapPageId pageId) {
  BufferPoolPage bufferPage = fetchPageAction.execute(pageId);
  SpaceMapPage fsmPage;
  std::memcpy(fsmPage.data(), bufferPage.data(), SPACE_MAP_PAGE_SIZE);
  return fsmPage;
}
