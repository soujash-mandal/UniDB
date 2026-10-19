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

    // Update policy state first: if it throws, the pin count remains unchanged.
    if (frame.getPinCount() == 1) {
      evictionPolicy.SetEvictable(pageId, true);
    }
    frame.unpin();
    return;
  }
  throw std::runtime_error("Page not found in buffer pool");
}
