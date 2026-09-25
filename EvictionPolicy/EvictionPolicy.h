#pragma once

#include <optional>

#include "EvictionPolicyPageId.h"

class EvictionPolicy {
public:
  virtual ~EvictionPolicy() = default;
  virtual void RecordAccess(EvictionPolicyPageId pageId) = 0;
  virtual void SetEvictable(EvictionPolicyPageId pageId, bool evictable) = 0;
  virtual std::optional<EvictionPolicyPageId> Evict() = 0;
  virtual void Remove(EvictionPolicyPageId pageId) = 0;
};
