#include "NewPageAction.h"

#include <stdexcept>

NewPageAction::NewPageAction(BufferPool &bufferPool,
                             AllocatePagePort &allocatePagePort,
                             EvictionPolicy &evictionPolicy)
    : bufferPool(bufferPool), allocatePagePort(allocatePagePort),
      evictionPolicy(evictionPolicy) {}

Page &NewPageAction::execute() {
  PageId pageId = allocatePagePort.allocatePage();
  // 1. First try to find an unused frame.
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied()) {
      frame.setPageId(pageId);
      frame.setOccupied(true);
      frame.setDirty(true);
      frame.pin();
      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
      return frame.getPage();
    }
  }

  // 2. No free frame. Try to evict a page.
  std::optional<PageId> victimPageId = evictionPolicy.Evict();
  if (!victimPageId.has_value()) {
    throw std::runtime_error("Buffer pool is full and no page is evictable");
  }

  // 3. Find the frame containing the victim page.
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (frame.isOccupied() && frame.getPageId() == victimPageId.value()) {
      frame.setPageId(pageId);
      frame.setOccupied(true);
      frame.setDirty(true);
      frame.pin();
      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
      return frame.getPage();
    }
  }

  throw std::runtime_error("Evicted page frame not found");
}
