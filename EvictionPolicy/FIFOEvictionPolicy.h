#pragma once

#include <list>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "EvictionPolicy.h"

class FIFOEvictionPolicy : public EvictionPolicy {
public:
  void RecordAccess(EvictionPageId pageId);
  void SetEvictable(EvictionPageId pageId, bool evictable);
  std::optional<EvictionPageId> Evict();
  void Remove(EvictionPageId pageId);

private:
  std::list<EvictionPageId> pages_;
  std::unordered_map<EvictionPageId, std::list<EvictionPageId>::iterator>
      pagePositions_;
  std::unordered_set<EvictionPageId> evictablePages_;
};
