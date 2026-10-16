#include "NewPageAction.h"

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
    if (!frame.isOccupied()) {
      const BufferPoolPageId pageId = allocatePagePort.allocatePage();
      frame.setPage(BufferPoolPage{});
      frame.setPageId(pageId);
      frame.setOccupied(true);
      frame.setDirty(true);
      frame.pin();

      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
      return pageId;
    }
  }

  const std::optional<BufferPoolPageId> victimPageId = evictionPolicy.Evict();
  if (!victimPageId.has_value()) {
    throw std::runtime_error("Buffer pool is full and no page is evictable");
  }

  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied() || frame.getPageId() != victimPageId.value()) {
      continue;
    }

    try {
      if (frame.isDirty()) {
        writePagePort.writePage(frame.getPageId(), frame.getPage());
        frame.setDirty(false);
      }

      // Do not discard the victim until replacement allocation succeeds.
      const BufferPoolPageId pageId = allocatePagePort.allocatePage();
      evictionPolicy.Remove(victimPageId.value());

      // A new logical page must never inherit the evicted page's bytes.
      frame.setPage(BufferPoolPage{});
      frame.setPageId(pageId);
      frame.setOccupied(true);
      frame.setDirty(true);
      frame.pin();

      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
      return pageId;
    } catch (...) {
      // Evict() removes the victim from policy state. Keep the old resident
      // frame reachable if writeback or page allocation fails.
      evictionPolicy.RecordAccess(victimPageId.value());
      evictionPolicy.SetEvictable(victimPageId.value(), true);
      throw;
    }
  }

  throw std::runtime_error(
      "Eviction policy returned a page not present in buffer pool");
}
