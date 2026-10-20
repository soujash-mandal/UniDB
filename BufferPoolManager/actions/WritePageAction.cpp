#include "WritePageAction.h"
#include <mutex>

#include <stdexcept>

WritePageAction::WritePageAction(BufferPool &bufferPool)
    : bufferPool(bufferPool) {}

void WritePageAction::execute(BufferPoolPageId pageId, BufferPoolPage page) {
  std::lock_guard<std::mutex> lock(bufferPool.getSynchronizationMutex());
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (frame.isOccupied() && frame.getPageId() == pageId) {
      frame.setPage(page);
      frame.setDirty(true);
      return;
    }
  }

  throw std::runtime_error("Page not found in buffer pool");
}
