#pragma once
#include "../port/CreateSpaceMapPagePort.h"
class CreateSpaceMapPageAction;
class CreateSpaceMapPageAdapter final : public CreateSpaceMapPagePort {
public:
  explicit CreateSpaceMapPageAdapter(CreateSpaceMapPageAction &action);
  uint32_t createSpaceMapPage() override;

private:
  CreateSpaceMapPageAction &action;
};
