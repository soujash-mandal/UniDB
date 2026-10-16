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
  // 1. Check whether the page is already in memory.
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (frame.isOccupied() && frame.getPageId() == pageId) {
      frame.pin();
      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
      return frame.getPage();
    }
  }

  // 2. Use an empty frame if one is available.
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

  // 3. The pool is full, so choose a page that can be replaced.
  const std::optional<BufferPoolPageId> victimPageId = evictionPolicy.Evict();
  if (!victimPageId.has_value()) {
    throw std::runtime_error("Buffer pool is full and no page is evictable");
  }

  // Evict() also removes this page from the eviction policy. If I/O fails,
  // we must add it back because its frame will still contain the old page.
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied() || frame.getPageId() != victimPageId.value()) {
      continue;
    }

    try {
      // 4. Save changes before replacing a dirty page.
      if (frame.isDirty()) {
        writePagePort.writePage(frame.getPageId(), frame.getPage());
        frame.setDirty(false);
      }

      // 5. Read the requested page before changing the frame. If this read
      // fails, the frame still contains the old page.
      BufferPoolPage page = readPagePort.readDiskPage(pageId);

      // 6. Replace the old page only after the required I/O has succeeded.
      evictionPolicy.Remove(victimPageId.value());
      frame.setPage(page);
      frame.setPageId(pageId);
      frame.setOccupied(true);
      frame.setDirty(false);
      frame.pin();

      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
      return frame.getPage();
    } catch (...) {
      // Evict() removed the victim from FIFO before I/O started. Put it back
      // so it can be chosen again; then pass the original error to the caller.
      evictionPolicy.RecordAccess(victimPageId.value());
      evictionPolicy.SetEvictable(victimPageId.value(), true);
      throw;
    }
  }

  throw std::runtime_error(
      "Eviction policy returned a page not present in buffer pool");
}
