#pragma once

#include "../../RdbMetadataService/actions/AllocateNextTableIdAction.h"
#include "../port/AllocateNextTableIdPort.h"

class AllocateNextTableIdAdapter final : public AllocateNextTableIdPort {
public:
  explicit AllocateNextTableIdAdapter(AllocateNextTableIdAction &action);
  uint32_t allocateNextTableId(uint32_t metadataPageId) override;

private:
  AllocateNextTableIdAction &action;
};
