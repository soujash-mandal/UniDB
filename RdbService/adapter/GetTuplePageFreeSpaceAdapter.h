#pragma once

#include "../port/GetTuplePageFreeSpacePort.h"

class GetTuplePageFreeSpaceAction;

namespace RdbService {

class GetTuplePageFreeSpaceAdapter final : public GetTuplePageFreeSpacePort {
public:
  explicit GetTuplePageFreeSpaceAdapter(GetTuplePageFreeSpaceAction &action);

  uint32_t getFreeSpace(uint32_t tuplePageId) override;

private:
  GetTuplePageFreeSpaceAction &action;
};

} // namespace RdbService
