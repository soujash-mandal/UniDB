#include "CreateSpaceMapPageAdapter.h"
#include "../../SpaceMapService/actions/CreateSpaceMapPageAction.h"
CreateSpaceMapPageAdapter::CreateSpaceMapPageAdapter(CreateSpaceMapPageAction &action)
    : action(action) {}
uint32_t CreateSpaceMapPageAdapter::createSpaceMapPage() {
  return action.execute();
}
