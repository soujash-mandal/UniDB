#include "UpdateFreeSpaceAdapter.h"

#include "../../SpaceMapService/actions/UpdateFreeSpaceAction.h"

namespace RdbService {

UpdateFreeSpaceAdapter::UpdateFreeSpaceAdapter(UpdateFreeSpaceAction &action)
    : action(action) {}

void UpdateFreeSpaceAdapter::updateFreeSpace(uint32_t spaceMapPageId,
                                             uint32_t tuplePageId,
                                             uint32_t freeSpace) {
  action.execute(spaceMapPageId, tuplePageId, freeSpace);
}

} // namespace RdbService
