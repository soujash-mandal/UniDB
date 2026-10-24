#include "FindPageWithSpaceAdapter.h"
#include "../../SpaceMapService/actions/FindPageWithSpaceAction.h"
FindPageWithSpaceAdapter::FindPageWithSpaceAdapter(FindPageWithSpaceAction &action)
    : action(action) {}
RdbFindPageWithSpaceResult FindPageWithSpaceAdapter::findPageWithSpace(
    uint32_t rootSpaceMapPageId, uint32_t requiredSpace) {
  const auto result = action.execute(rootSpaceMapPageId, requiredSpace);
  return {result.tuplePageId, result.spaceMapPageId};
}
