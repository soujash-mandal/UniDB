#pragma once

#include "../../actions/DeletePageAction.h"
#include "../../../EvictionPolicy/FIFOEvictionPolicy.h"

#include <stdexcept>

class FakeDeletePagePort final : public DeallocatePagePort {
public:
  uint32_t calls = 0;
  uint32_t pageId = 0;
  bool fail = false;

  void deallocatePage(BufferPoolPageId requestedPageId) override {
    ++calls;
    pageId = requestedPageId;
    if (fail) {
      throw std::runtime_error("deallocation failed");
    }
  }
};
