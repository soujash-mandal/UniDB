#pragma once

#include "../../FreePageManager/actions/AllocatePageAction.h"
#include "../port/AllocatePagePort.h"

class AllocatePageAdapter : public AllocatePagePort {
public:
  explicit AllocatePageAdapter(AllocatePageAction &allocatePageAction);

  PageId allocatePage() override;

private:
  AllocatePageAction &allocatePageAction;
};
