#include "UnpinPageAction.h"

#include <stdexcept>

UnpinPageAction::UnpinPageAction(BufferPool &bufferPool,
                                 EvictionPolicy &evictionPolicy)
    : bufferPool(bufferPool), evictionPolicy(evictionPolicy) {}

void UnpinPageAction::execute(BufferPoolPageId pageId) {
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (frame.isOccupied() && frame.getPageId() == pageId) {
      frame.unpin();
      if (frame.getPinCount() == 0) {
        evictionPolicy.SetEvictable(pageId, true);
      }
      return;
    }
  }
  throw std::runtime_error("Page not found in buffer pool");
}
