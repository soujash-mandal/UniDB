#include "NewPageAction.h"

#include <optional>
#include <stdexcept>

NewPageAction::NewPageAction(BufferPool &bufferPool,
                             AllocatePagePort &allocatePagePort,
                             WritePagePort &writePagePort,
                             EvictionPolicy &evictionPolicy)
    : bufferPool(bufferPool), allocatePagePort(allocatePagePort),
      writePagePort(writePagePort), evictionPolicy(evictionPolicy) {}

Page &NewPageAction::execute() {
  // 1. First try to find an unused frame.
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied()) {
      PageId pageId = allocatePagePort.allocatePage();
      frame.setPageId(pageId);
      frame.setOccupied(true);
      frame.setDirty(true);
      frame.pin();
      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
      return frame.getPage();
    }
  }

  // 2. Buffer pool is full.
  // Find a page that can be evicted.
  std::optional<PageId> victimPageId = evictionPolicy.Evict();
  if (!victimPageId.has_value()) {
    throw std::runtime_error("Buffer pool is full and no page is evictable");
  }

  // 3. Find the frame containing the victim.
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied() || frame.getPageId() != victimPageId.value()) {
      continue;
    }

    // 4. Persist the victim if it is dirty.
    if (frame.isDirty()) {
      writePagePort.writePage(frame.getPageId(), frame.getPage());
      frame.setDirty(false);
    }

    // 5. Allocate the new page only after we know
    //    that the frame can actually be reused.
    PageId pageId = allocatePagePort.allocatePage();

    // 6. Replace the victim with the new page.
    frame.setPageId(pageId);
    frame.setOccupied(true);
    frame.setDirty(true);
    frame.pin();

    // 7. Register the new page.
    evictionPolicy.RecordAccess(pageId);
    evictionPolicy.SetEvictable(pageId, false);

    return frame.getPage();
  }

  throw std::runtime_error(
      "Eviction policy returned a page not present in buffer pool");
}

// todo: NewPageAction → add evictionPolicy.Remove(victimPageId) for
// consistency. -> need to check first