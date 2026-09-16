#include "NewPageAction.h"

#include <stdexcept>

NewPageAction::NewPageAction(BufferPool &bufferPool) : bufferPool(bufferPool) {}

Page &NewPageAction::execute(PageId pageId) {
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied()) {
      frame.setPageId(pageId);
      frame.setOccupied(true);
      frame.setDirty(true);
      frame.pin();
      return frame.getPage();
    }
  }

  throw std::runtime_error("Buffer pool is full");
}
