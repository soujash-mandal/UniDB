#pragma once

#include <functional>
#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>

#include "../../actions/NewPageAction.h"
#include "../../actions/UnpinPageAction.h"

class UnpinTestEvictionPolicy : public EvictionPolicy {
public:
  int recordAccessCount = 0;
  int setEvictableCount = 0;
  BufferPoolPageId lastPageId = 0;
  bool lastEvictable = false;
  bool shouldThrowOnEvictable = false;
  bool shouldThrowAfterCallback = false;
  std::function<void()> onMakeEvictable;
  std::optional<BufferPoolPageId> victim;

  void RecordAccess(BufferPoolPageId) override { ++recordAccessCount; }
  void SetEvictable(BufferPoolPageId pageId, bool evictable) override {
    ++setEvictableCount;
    lastPageId = pageId;
    lastEvictable = evictable;
    if (shouldThrowOnEvictable) {
      shouldThrowOnEvictable = false;
      throw std::runtime_error("simulated policy failure");
    }
    if (evictable && onMakeEvictable) {
      onMakeEvictable();
    }
    if (evictable && shouldThrowAfterCallback) {
      shouldThrowAfterCallback = false;
      throw std::runtime_error("simulated post-callback policy failure");
    }
  }
  std::optional<BufferPoolPageId> Evict() override { return victim; }
  void Remove(BufferPoolPageId) override {}
};

inline void OccupyFrame(BufferPool &pool, BufferPoolPageId pageId,
                        uint32_t pinCount) {
  Frame &frame = pool.getFrame(0);
  frame.setPageId(pageId);
  frame.setOccupied(true);
  for (uint32_t i = 0; i < pinCount; ++i) {
    frame.pin();
  }
}

class UnpinTestAllocatePort : public AllocatePagePort {
public:
  BufferPoolPageId allocatePage() override { return 8; }
};

class UnpinTestWritePort : public WritePagePort {
public:
  void writePage(BufferPoolPageId, BufferPoolPage) override {}
};
