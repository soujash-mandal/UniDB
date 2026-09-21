#include "FetchPageAction.h"

#include <optional>
#include <stdexcept>

FetchPageAction::FetchPageAction(BufferPool &bufferPool,
                                 ReadPagePort &readPagePort,
                                 WritePagePort &writePagePort,
                                 EvictionPolicy &evictionPolicy)
    : bufferPool(bufferPool), readPagePort(readPagePort),
      writePagePort(writePagePort), evictionPolicy(evictionPolicy) {}

Page &FetchPageAction::execute(PageId pageId) {

  // 1. Check whether page is already in the buffer pool
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (frame.isOccupied() && frame.getPageId() == pageId) {
      frame.pin();
      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
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
      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
      return frame.getPage();
    }
  }

  // 3. Buffer pool is full.
  // Ask the eviction policy which page should be removed.
  std::optional<PageId> victimPageId = evictionPolicy.Evict();
  if (!victimPageId.has_value()) {
    throw std::runtime_error("Buffer pool is full and no page is evictable");
  }

  // 4. Find the frame containing the victim page
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied() || frame.getPageId() != victimPageId.value()) {
      continue;
    }

    // 5. If the victim is dirty, write it to disk first.
    if (frame.isDirty()) {
      writePagePort.writePage(frame.getPageId(), frame.getPage());
      frame.setDirty(false);
    }

    // 6. Remove the old page from the eviction policy.
    evictionPolicy.Remove(victimPageId.value());

    // 7. Load the requested page into the same frame.
    readPagePort.readPage(pageId, frame.getPage());
    frame.setPageId(pageId);
    frame.setOccupied(true);
    frame.setDirty(false);
    frame.pin();

    // 8. Add the new page to the eviction policy.
    evictionPolicy.RecordAccess(pageId);
    evictionPolicy.SetEvictable(pageId, false);

    return frame.getPage();
  }

  throw std::runtime_error(
      "Eviction policy returned a page not present in buffer pool");
}