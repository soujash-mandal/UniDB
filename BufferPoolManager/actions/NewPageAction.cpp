#include "NewPageAction.h"

#include <exception>
#include <optional>
#include <stdexcept>

NewPageAction::NewPageAction(BufferPool &bufferPool,
                             AllocatePagePort &allocatePagePort,
                             WritePagePort &writePagePort,
                             EvictionPolicy &evictionPolicy)
    : bufferPool(bufferPool), allocatePagePort(allocatePagePort),
      writePagePort(writePagePort), evictionPolicy(evictionPolicy) {}

BufferPoolPageId NewPageAction::execute() {
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (frame.isOccupied()) {
      continue;
    }

    const BufferPoolPageId pageId = allocatePagePort.allocatePage();
    try {
      frame.setPage(BufferPoolPage{});
      frame.setPageId(pageId);
      frame.setOccupied(true);
      frame.setDirty(true);
      frame.pin();

      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
      return pageId;
    } catch (...) {
      const std::exception_ptr originalError = std::current_exception();
      try {
        evictionPolicy.Remove(pageId);
      } catch (...) {
      }
      if (frame.getPinCount() > 0) {
        frame.unpin();
      }
      frame.setOccupied(false);
      frame.setDirty(false);
      frame.setPageId(0);
      frame.setPage(BufferPoolPage{});
      std::rethrow_exception(originalError);
    }
  }

  // Evict() removes the victim from FIFO, so restore its registration if any
  // later step fails.
  const std::optional<BufferPoolPageId> victimPageId = evictionPolicy.Evict();
  if (!victimPageId.has_value()) {
    throw std::runtime_error("Buffer pool is full and no page is evictable");
  }

  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied() || frame.getPageId() != victimPageId.value()) {
      continue;
    }

    const BufferPoolPage oldPage = frame.getPage();
    const BufferPoolPageId oldPageId = frame.getPageId();
    const bool oldDirty = frame.isDirty();
    const bool oldOccupied = frame.isOccupied();
    const uint32_t oldPinCount = frame.getPinCount();
    bool oldPageWritten = false;
    BufferPoolPageId newPageId = 0;
    bool newPageAllocated = false;

    try {
      if (oldDirty) {
        writePagePort.writePage(oldPageId, oldPage);
        oldPageWritten = true;
        frame.setDirty(false);
      }

      newPageId = allocatePagePort.allocatePage();
      newPageAllocated = true;
      evictionPolicy.Remove(oldPageId);

      frame.setPage(BufferPoolPage{});
      frame.setPageId(newPageId);
      frame.setOccupied(true);
      frame.setDirty(true);
      frame.pin();

      evictionPolicy.RecordAccess(newPageId);
      evictionPolicy.SetEvictable(newPageId, false);
      return newPageId;
    } catch (...) {
      const std::exception_ptr originalError = std::current_exception();
      if (newPageAllocated) {
        try {
          evictionPolicy.Remove(newPageId);
        } catch (...) {
        }
      }

      // Restore the old resident page. A successful writeback means it is now
      // clean; failed writeback must leave it dirty and retryable.
      while (frame.getPinCount() > 0) {
        frame.unpin();
      }
      frame.setPage(oldPage);
      frame.setPageId(oldPageId);
      frame.setOccupied(oldOccupied);
      frame.setDirty(oldDirty && !oldPageWritten);
      for (uint32_t i = 0; i < oldPinCount; ++i) {
        frame.pin();
      }

      try {
        evictionPolicy.RecordAccess(oldPageId);
        evictionPolicy.SetEvictable(oldPageId, true);
      } catch (...) {
        // Preserve the original operation's exception if rollback itself fails.
      }
      std::rethrow_exception(originalError);
    }
  }

  throw std::runtime_error(
      "Eviction policy returned a page not present in buffer pool");
}
