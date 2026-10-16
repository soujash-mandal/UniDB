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

  // The pool is full. Evict() chooses a victim and removes it from FIFO.
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
      // Save the old page first if it has changes that are not on disk yet.
      if (frame.isDirty()) {
        writePagePort.writePage(frame.getPageId(), frame.getPage());
        frame.setDirty(false);
      }

      // Get a new page ID before changing the frame. If allocation fails,
      // the frame still holds the old page.
      const BufferPoolPageId pageId = allocatePagePort.allocatePage();
      evictionPolicy.Remove(victimPageId.value());

      // Start the new page with empty bytes; do not reuse the victim's data.
      frame.setPage(BufferPoolPage{});
      frame.setPageId(pageId);
      frame.setOccupied(true);
      frame.setDirty(true);
      frame.pin();

      evictionPolicy.RecordAccess(pageId);
      evictionPolicy.SetEvictable(pageId, false);
      return pageId;
    } catch (...) {
      // If saving or allocating fails, the old page is still in this frame.
      // Add it back to FIFO so it can be selected again, then rethrow the error.
      evictionPolicy.RecordAccess(victimPageId.value());
      evictionPolicy.SetEvictable(victimPageId.value(), true);
      throw;
    }
  }

  throw std::runtime_error(
      "Eviction policy returned a page not present in buffer pool");
}
