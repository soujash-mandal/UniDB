#pragma once

#include "../../BufferPoolManager/actions/DiscardPageAction.h"
#include "../actions/FreePageAction.h"

class BufferPoolDiscardPageAdapter final : public DiscardPagePort {
public:
  explicit BufferPoolDiscardPageAdapter(DiscardPageAction &discardPageAction)
      : discardPageAction(discardPageAction) {}
  void discardPage(uint32_t pageId) override {
    discardPageAction.execute(pageId);
  }

private:
  DiscardPageAction &discardPageAction;
};
