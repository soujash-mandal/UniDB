#pragma once

#include "../../FreePageManager/actions/FreePageAction.h"
#include "../port/FreeTuplePagePort.h"

class FreePageAdapter final : public FreeTuplePagePort {
public:
  explicit FreePageAdapter(FreePageAction &freePageAction)
      : freePageAction(freePageAction) {}
  void freePage(uint32_t pageId) override { freePageAction.execute(pageId); }

private:
  FreePageAction &freePageAction;
};
