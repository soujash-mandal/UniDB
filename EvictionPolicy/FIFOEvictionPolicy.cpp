#include "FIFOEvictionPolicy.h"

void FIFOEvictionPolicy::RecordAccess(EvictionPolicyPageId pageId) {
  if (pagePositions_.contains(pageId)) {
    return;
  }
  pages_.push_back(pageId);
  pagePositions_[pageId] = std::prev(pages_.end());
}

void FIFOEvictionPolicy::SetEvictable(EvictionPolicyPageId pageId,
                                      bool evictable) {
  if (!pagePositions_.contains(pageId)) {
    return;
  }
  if (evictable) {
    evictablePages_.insert(pageId);
  } else {
    evictablePages_.erase(pageId);
  }
}

std::optional<EvictionPolicyPageId> FIFOEvictionPolicy::Evict() {
  for (auto it = pages_.begin(); it != pages_.end(); ++it) {
    EvictionPolicyPageId pageId = *it;
    if (evictablePages_.contains(pageId)) {
      evictablePages_.erase(pageId);
      pagePositions_.erase(pageId);
      pages_.erase(it);
      return pageId;
    }
  }
  return std::nullopt;
}

void FIFOEvictionPolicy::Remove(EvictionPolicyPageId pageId) {
  auto it = pagePositions_.find(pageId);
  if (it == pagePositions_.end()) {
    return;
  }
  evictablePages_.erase(pageId);
  pages_.erase(it->second);
  pagePositions_.erase(it);
}
