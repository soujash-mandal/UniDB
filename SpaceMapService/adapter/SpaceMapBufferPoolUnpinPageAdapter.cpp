#include "SpaceMapBufferPoolUnpinPageAdapter.h"

SpaceMapBufferPoolUnpinPageAdapter::SpaceMapBufferPoolUnpinPageAdapter(
    UnpinPageAction &unpinPageAction)
    : unpinPageAction(unpinPageAction) {}

void SpaceMapBufferPoolUnpinPageAdapter::unpinPage(SpaceMapPageId pageId) {
  unpinPageAction.execute(pageId);
}
