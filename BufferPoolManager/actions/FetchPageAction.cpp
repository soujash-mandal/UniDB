#include "FetchPageAction.h"

#include <optional>
#include <stdexcept>

FetchPageAction::FetchPageAction(BufferPool &bufferPool,
                                 ReadPagePort &readPagePort,
                                 WritePagePort &writePagePort,
                                 EvictionPolicy &evictionPolicy)
    : bufferPool(bufferPool), readPagePort(readPagePort),
      writePagePort(writePagePort), evictionPolicy(evictionPolicy) {}

BufferPoolPage FetchPageAction::execute(BufferPoolPageId pageId) {

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
      BufferPoolPage page = readPagePort.readDiskPage(pageId);
      frame.setPageId(pageId);
      frame.setPage(page);
      frame.setOccupied(true);
      frame.setDirty(false);
      frame.pin();

      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
      return frame.getPage();
    }
  }

  // 3. Buffer pool is full. Evict() may remove its victim from policy state,
  // so restore that victim if writeback or the requested read fails.
  std::optional<BufferPoolPageId> victimPageId = evictionPolicy.Evict();
  if (!victimPageId.has_value()) {
    throw std::runtime_error("Buffer pool is full and no page is evictable");
  }

  // 4. Find the frame containing the victim page.
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied() || frame.getPageId() != victimPageId.value()) {
      continue;
    }

    BufferPoolPage page;
    try {
      // 5. Persist dirty victim data before attempting to replace the frame.
      if (frame.isDirty()) {
        writePagePort.writePage(frame.getPageId(), frame.getPage());
        frame.setDirty(false);
      }

      // 6. Read into a temporary first; preserve the victim on read failure.
      page = readPagePort.readDiskPage(pageId);
    } catch (...) {
      // Restore a selectable victim so the caller can retry this fetch.
      evictionPolicy.RecordAccess(victimPageId.value());
      evictionPolicy.SetEvictable(victimPageId.value(),
                                  frame.getPinCount() == 0);
      throw;
    }

    // 7. Commit only after both required I/O operations succeed.
    // Some policies remove their victim inside Evict(); Remove is intentionally
    // idempotent for policies that retain the victim until explicitly removed.
    evictionPolicy.Remove(victimPageId.value());
    frame.setPage(page);
    frame.setPageId(pageId);
    frame.setOccupied(true);
    frame.setDirty(false);
    frame.pin();

    // 8. Register the new resident page as non-evictable while it is pinned.
    evictionPolicy.RecordAccess(pageId);
    evictionPolicy.SetEvictable(pageId, false);

    return frame.getPage();
  }

  throw std::runtime_error(
      "Eviction policy returned a page not present in buffer pool");
}