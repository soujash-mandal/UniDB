#include "FlushPageAction.h"

#include <stdexcept>

FlushPageAction::FlushPageAction(BufferPool &bufferPool,
                                 WritePagePort &writePagePort)
    : bufferPool(bufferPool), writePagePort(writePagePort) {}

void FlushPageAction::execute(PageId pageId) {
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (frame.isOccupied() && frame.getPageId() == pageId) {
      writePagePort.writePage(pageId, frame.getPage());
      frame.setDirty(false);
      return;
    }
  }

  throw std::runtime_error("Page not found in buffer pool");
}
