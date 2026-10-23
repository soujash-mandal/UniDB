#include "DiscardPageAction.h"

#include <stdexcept>

DiscardPageAction::DiscardPageAction(BufferPool &bufferPool,
                                     EvictionPolicy &evictionPolicy)
    : bufferPool(bufferPool), evictionPolicy(evictionPolicy) {}

void DiscardPageAction::execute(BufferPoolPageId pageId) {
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied() || frame.getPageId() != pageId) {
      continue;
    }
    if (frame.getPinCount() != 0) {
      throw std::runtime_error("Cannot discard a pinned page");
    }
    evictionPolicy.Remove(pageId);
    frame.setOccupied(false);
    frame.setDirty(false);
    return;
  }
}
