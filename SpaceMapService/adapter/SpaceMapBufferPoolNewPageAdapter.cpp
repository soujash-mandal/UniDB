#include "SpaceMapBufferPoolNewPageAdapter.h"

SpaceMapBufferPoolNewPageAdapter::SpaceMapBufferPoolNewPageAdapter(
    NewPageAction &newPageAction)
    : newPageAction(newPageAction) {}

SpaceMapPageId SpaceMapBufferPoolNewPageAdapter::newPage() {
  return newPageAction.execute();
}
