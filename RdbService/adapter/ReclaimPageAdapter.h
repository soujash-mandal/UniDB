#pragma once

#include "../port/ReclaimPagePort.h"

class DeletePageAction;

namespace RdbService {

class ReclaimPageAdapter final : public ReclaimPagePort {
public:
  explicit ReclaimPageAdapter(DeletePageAction &deletePageAction);
  void reclaimPage(uint32_t pageId) override;

private:
  DeletePageAction &deletePageAction;
};

} // namespace RdbService
