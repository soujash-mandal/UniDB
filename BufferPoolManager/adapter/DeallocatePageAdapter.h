#pragma once

#include "../../FreePageManager/actions/DeallocatePageAction.h"
#include "../port/DeallocatePagePort.h"

class DeallocatePageAdapter final : public DeallocatePagePort {
public:
  explicit DeallocatePageAdapter(DeallocatePageAction &action);
  void deallocatePage(BufferPoolPageId pageId) override;

private:
  DeallocatePageAction &action;
};
