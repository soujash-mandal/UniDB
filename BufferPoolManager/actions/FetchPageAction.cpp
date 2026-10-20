#include "FetchPageAction.h"
#include <mutex>

#include <optional>
#include <stdexcept>

FetchPageAction::FetchPageAction(BufferPool &bufferPool,
                                 ReadPagePort &readPagePort,
                                 WritePagePort &writePagePort,
                                 EvictionPolicy &evictionPolicy)
    : bufferPool(bufferPool), readPagePort(readPagePort),
      writePagePort(writePagePort), evictionPolicy(evictionPolicy) {}

BufferPoolPage FetchPageAction::execute(BufferPoolPageId pageId) {
  std::lock_guard<std::mutex> lock(bufferPool.getSynchronizationMutex());
  // 1. Check whether the page is already in memory.
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (frame.isOccupied() && frame.getPageId() == pageId) {
      // Update policy state before pinning: policy methods may throw, and the
      // caller cannot unpin a page when fetchPage itself fails.
      try {
        evictionPolicy.RecordAccess(pageId);
        evictionPolicy.SetEvictable(pageId, false);
      } catch (...) {
        try {
          evictionPolicy.SetEvictable(pageId, frame.getPinCount() == 0);
        } catch (...) {
          // Preserve the original eviction-policy failure.
        }
        throw;
      }
      frame.pin();
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

      try {
        evictionPolicy.RecordAccess(pageId);
        evictionPolicy.SetEvictable(pageId, false);
      } catch (...) {
        try {
          evictionPolicy.Remove(pageId);
        } catch (...) {
          // Preserve the original eviction-policy failure.
        }
        frame.setOccupied(false);
        frame.setDirty(false);
        throw;
      }

      frame.pin();
      return frame.getPage();
    }
  }

  // 3. The pool is full, so choose a page that can be replaced.
  const std::optional<BufferPoolPageId> victimPageId = evictionPolicy.Evict();
  if (!victimPageId.has_value()) {
    throw std::runtime_error("Buffer pool is full and no page is evictable");
  }

  // Evict() removes the victim from the policy. Preserve its frame contents so
  // policy failures can roll back the replacement as well as the mapping.
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied() || frame.getPageId() != victimPageId.value()) {
      continue;
    }

    const BufferPoolPage oldPage = frame.getPage();
    const BufferPoolPageId oldPageId = frame.getPageId();
    const bool oldDirty = frame.isDirty();
    bool restoredDirty = oldDirty;

    try {
      // 4. Save changes before replacing a dirty page.
      if (frame.isDirty()) {
        writePagePort.writePage(frame.getPageId(), frame.getPage());
        frame.setDirty(false);
        restoredDirty = false;
      }

      // 5. Read the requested page before changing the frame.
      BufferPoolPage page = readPagePort.readDiskPage(pageId);

      // 6. Replace the old page only after required I/O has succeeded.
      evictionPolicy.Remove(victimPageId.value());
      frame.setPage(page);
      frame.setPageId(pageId);
      frame.setOccupied(true);
      frame.setDirty(false);

      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
      frame.pin();
      return frame.getPage();
    } catch (...) {
      // Remove any partial mapping for the requested page, then restore the
      // victim frame and make it evictable again for a later retry.
      try {
        evictionPolicy.Remove(pageId);
      } catch (...) {
        // Preserve the original failure.
      }
      frame.setPage(oldPage);
      frame.setPageId(oldPageId);
      frame.setOccupied(true);
      frame.setDirty(restoredDirty);
      try {
        evictionPolicy.RecordAccess(oldPageId);
        evictionPolicy.SetEvictable(oldPageId, true);
      } catch (...) {
        // Preserve the original failure.
      }
      throw;
    }
  }

  throw std::runtime_error(
      "Eviction policy returned a page not present in buffer pool");
}
