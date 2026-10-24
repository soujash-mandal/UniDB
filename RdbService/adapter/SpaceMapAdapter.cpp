#include "SpaceMapAdapter.h"

#include "../../SpaceMapService/actions/CreateSpaceMapPageAction.h"
#include "../../SpaceMapService/actions/FindPageWithSpaceAction.h"
#include "../../SpaceMapService/actions/GetTablePagesAction.h"

SpaceMapAdapter::SpaceMapAdapter(
    CreateSpaceMapPageAction &createSpaceMapPageAction,
    FindPageWithSpaceAction &findPageWithSpaceAction,
    GetTablePagesAction &getTablePagesAction)
    : createSpaceMapPageAction(createSpaceMapPageAction),
      findPageWithSpaceAction(findPageWithSpaceAction),
      getTablePagesAction(getTablePagesAction) {}

uint32_t SpaceMapAdapter::createSpaceMapPage() {
  return createSpaceMapPageAction.execute();
}

RdbFindPageWithSpaceResult SpaceMapAdapter::findPageWithSpace(
    uint32_t rootSpaceMapPageId, uint32_t requiredSpace) {
  const auto result =
      findPageWithSpaceAction.execute(rootSpaceMapPageId, requiredSpace);
  return {result.tuplePageId, result.spaceMapPageId};
}

std::vector<uint32_t>
SpaceMapAdapter::getTablePages(uint32_t rootSpaceMapPageId) {
  return getTablePagesAction.execute(rootSpaceMapPageId);
}
