#include "FetchPageAction.h"

FetchPageAction::FetchPageAction(BufferPool &bufferPool,
                                 ReadPagePort &readPagePort)
    : bufferPool(bufferPool), readPagePort(readPagePort) {}

Page &FetchPageAction::execute(PageId pageId) {

  // 1. Check whether page is already in the buffer pool
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (frame.isOccupied() && frame.getPageId() == pageId) {
      frame.pin();
      return frame.getPage();
    }
  }

  // 2. Find an empty frame
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied()) {
      readPagePort.readPage(pageId, frame.getPage());
      frame.setPageId(pageId);
      frame.setOccupied(true);
      frame.setDirty(false);
      frame.pin();
      return frame.getPage();
    }
  }

  throw std::runtime_error("Buffer pool is full");
}
