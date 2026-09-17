#pragma once

#include <list>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "EvictionPolicy.h"

class FIFOEvictionPolicy : public EvictionPolicy {
public:
  void RecordAccess(PageId pageId);
  void SetEvictable(PageId pageId, bool evictable);
  std::optional<PageId> Evict();
  void Remove(PageId pageId);

private:
  std::list<PageId> pages_;
  std::unordered_map<PageId, std::list<PageId>::iterator> pagePositions_;
  std::unordered_set<PageId> evictablePages_;
};
