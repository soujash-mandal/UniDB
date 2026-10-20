#include "UnpinPageAction.h"

#include <mutex>
#include <stdexcept>

UnpinPageAction::UnpinPageAction(BufferPool &bufferPool,
                                 EvictionPolicy &evictionPolicy)
    : bufferPool(bufferPool), evictionPolicy(evictionPolicy) {}

void UnpinPageAction::execute(BufferPoolPageId pageId) {
  std::lock_guard<std::mutex> lock(bufferPool.getSynchronizationMutex());

  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied() || frame.getPageId() != pageId) {
      continue;
    }

    if (frame.getPinCount() == 1) {
      // Keep the final pin until the policy accepts the evictability change.
      // The shared lock prevents replacement between this update and unpin.
      evictionPolicy.SetEvictable(pageId, true);
    }

    frame.unpin();
    return;
  }
  throw std::runtime_error("Page not found in buffer pool");
}
