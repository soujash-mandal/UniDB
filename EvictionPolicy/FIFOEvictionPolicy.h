#pragma once

#include <list>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "EvictionPolicy.h"

class FIFOEvictionPolicy : public EvictionPolicy {
public:
  void RecordAccess(EvictionPolicyPageId pageId);
  void SetEvictable(EvictionPolicyPageId pageId, bool evictable);
  std::optional<EvictionPolicyPageId> Evict();
  void Remove(EvictionPolicyPageId pageId);

private:
  std::list<EvictionPolicyPageId> pages_;
  std::unordered_map<EvictionPolicyPageId,
                     std::list<EvictionPolicyPageId>::iterator>
      pagePositions_;
  std::unordered_set<EvictionPolicyPageId> evictablePages_;
};
