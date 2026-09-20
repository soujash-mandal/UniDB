#pragma once

#include <optional>

#include "../core/PageId.h"

class EvictionPolicy {
public:
  virtual ~EvictionPolicy() = default;
  virtual void RecordAccess(PageId pageId) = 0;
  virtual void SetEvictable(PageId pageId, bool evictable) = 0;
  virtual std::optional<PageId> Evict() = 0;
  virtual void Remove(PageId pageId) = 0;
};
