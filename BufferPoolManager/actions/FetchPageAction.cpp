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
  // 1. Check whether page is already in the buffer pool.
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (frame.isOccupied() && frame.getPageId() == pageId) {
      frame.pin();
      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
      return frame.getPage();
    }
  }

  // 2. Find an empty frame.
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

  // 3. Buffer pool is full; select a victim.
  const std::optional<BufferPoolPageId> victimPageId = evictionPolicy.Evict();
  if (!victimPageId.has_value()) {
    throw std::runtime_error("Buffer pool is full and no page is evictable");
  }

  // 4. Find the frame containing the victim page. Evict() removes the victim
  // from the policy, so restore it if replacement I/O fails.
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied() || frame.getPageId() != *victimPageId) {
      continue;
    }

    try {
      // 5. Write dirty contents before replacing the frame.
      if (frame.isDirty()) {
        writePagePort.writePage(frame.getPageId(), frame.getPage());
        frame.setDirty(false);
      }

      // 6. Read first: if this fails, the original frame remains intact.
      BufferPoolPage page = readPagePort.readDiskPage(pageId);

      // 7. Replace the old resident page only after all required I/O succeeds.
      evictionPolicy.Remove(*victimPageId);
      frame.setPage(page);
      frame.setPageId(pageId);
      frame.setOccupied(true);
      frame.setDirty(false);
      frame.pin();

      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
      return frame.getPage();
    } catch (...) {
      // Evict() already removed this page from FIFO's internal state. Restore
      // it so the still-resident frame can be selected on the next attempt.
      evictionPolicy.RecordAccess(*victimPageId);
      evictionPolicy.SetEvictable(*victimPageId, true);
      throw;
    }
  }

  throw std::runtime_error(
      "Eviction policy returned a page not present in buffer pool");
}
