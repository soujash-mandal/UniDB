#pragma once

#include "../port/CalculateRequiredSpacePort.h"

class CalculateRequiredSpaceAction;

namespace RdbService {

class CalculateRequiredSpaceAdapter final : public CalculateRequiredSpacePort {
public:
  explicit CalculateRequiredSpaceAdapter(
      CalculateRequiredSpaceAction &action);

  uint32_t calculateRequiredSpace(
      const std::vector<char> &tupleData) override;

private:
  CalculateRequiredSpaceAction &action;
};

} // namespace RdbService
