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
      BufferPoolPageId pageId = allocatePagePort.allocatePage();
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

  std::optional<BufferPoolPageId> victimPageId = evictionPolicy.Evict();
  if (!victimPageId.has_value()) {
    throw std::runtime_error("Buffer pool is full and no page is evictable");
  }

  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (!frame.isOccupied() || frame.getPageId() != victimPageId.value()) {
      continue;
    }

    BufferPoolPageId pageId;
    try {
      // Persist dirty data before allocating a replacement. Clear dirty only
      // after the write succeeds, so a failed write remains retryable.
      if (frame.isDirty()) {
        writePagePort.writePage(frame.getPageId(), frame.getPage());
        frame.setDirty(false);
      }
      pageId = allocatePagePort.allocatePage();
    } catch (...) {
      // Evict() may remove its victim from policy state. Restore it on any
      // failure before replacing the frame, so a subsequent request can retry.
      evictionPolicy.RecordAccess(victimPageId.value());
      evictionPolicy.SetEvictable(victimPageId.value(),
                                  frame.getPinCount() == 0);
      throw;
    }

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
  }

  throw std::runtime_error(
      "Eviction policy returned a page not present in buffer pool");
}
