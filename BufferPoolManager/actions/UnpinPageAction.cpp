#include "UnpinPageAction.h"

#include <stdexcept>

UnpinPageAction::UnpinPageAction(BufferPool &bufferPool,
                                 EvictionPolicy &evictionPolicy)
    : bufferPool(bufferPool), evictionPolicy(evictionPolicy) {}

void UnpinPageAction::execute(BufferPoolPageId pageId) {
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied() || frame.getPageId() != pageId) {
      continue;
    }

    // Release the final pin before exposing the page to eviction. Otherwise,
    // replacement can reuse this frame before the old pin is decremented.
    if (frame.getPinCount() == 1) {
      frame.unpin();
      try {
        evictionPolicy.SetEvictable(pageId, true);
      } catch (...) {
        // Preserve the pin if the policy update fails, so the caller can retry.
        frame.pin();
        throw;
      }
      return;
    }

    frame.unpin();
    return;
  }
  throw std::runtime_error("Page not found in buffer pool");
}
