#include "UnpinPageAction.h"

UnpinPageAction::UnpinPageAction(BufferPool &bufferPool)
    : bufferPool(bufferPool) {}

void UnpinPageAction::execute(PageId pageId, bool dirty) {
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (frame.isOccupied() && frame.getPageId() == pageId) {
      frame.unpin();
      if (dirty) {
        frame.setDirty(true);
      }
      return;
    }
  }
  throw std::runtime_error("Page not found in buffer pool");
}
