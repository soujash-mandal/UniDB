#pragma once

#include <optional>

#include "EvictionPageId.h"

class EvictionPolicy {
public:
  virtual ~EvictionPolicy() = default;
  virtual void RecordAccess(EvictionPageId pageId) = 0;
  virtual void SetEvictable(EvictionPageId pageId, bool evictable) = 0;
  virtual std::optional<EvictionPageId> Evict() = 0;
  virtual void Remove(EvictionPageId pageId) = 0;
};
