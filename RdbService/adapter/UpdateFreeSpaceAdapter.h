#pragma once

#include "../port/UpdateFreeSpacePort.h"

class UpdateFreeSpaceAction;

namespace RdbService {

class UpdateFreeSpaceAdapter final : public UpdateFreeSpacePort {
public:
  explicit UpdateFreeSpaceAdapter(UpdateFreeSpaceAction &action);

  void updateFreeSpace(uint32_t spaceMapPageId, uint32_t tuplePageId,
                       uint32_t freeSpace) override;

private:
  UpdateFreeSpaceAction &action;
};

} // namespace RdbService
