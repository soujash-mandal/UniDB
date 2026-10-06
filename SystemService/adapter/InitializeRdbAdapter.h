#pragma once

#include "../../RdbService/actions/InitializeRdbAction.h"
#include "../port/InitializeRdbPort.h"

class InitializeRdbAdapter : public InitializeRdbPort {
public:
  explicit InitializeRdbAdapter(InitializeRdbAction &initializeRdbAction);
  RdbMetadataPageId initialize() override;

private:
  InitializeRdbAction &initializeRdbAction;
};
