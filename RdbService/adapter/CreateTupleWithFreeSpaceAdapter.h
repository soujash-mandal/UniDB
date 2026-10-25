#pragma once

#include "../port/CreateTupleWithFreeSpacePort.h"

class CreateTupleWithFreeSpaceAction;

namespace RdbService {

class CreateTupleWithFreeSpaceAdapter final
    : public CreateTupleWithFreeSpacePort {
public:
  explicit CreateTupleWithFreeSpaceAdapter(
      ::CreateTupleWithFreeSpaceAction &action);

  RdbCreateTupleWithFreeSpaceResult
  createTupleWithFreeSpace(uint32_t tuplePageId,
                           std::vector<char> tupleData) override;

private:
  ::CreateTupleWithFreeSpaceAction &action;
};

} // namespace RdbService
