#include "DeletePageAction.h"

#include <exception>
#include <mutex>
#include <stdexcept>

DeletePageAction::DeletePageAction(BufferPool &bufferPool,
                                   EvictionPolicy &evictionPolicy,
                                   DeallocatePagePort &deallocatePagePort)
    : bufferPool(bufferPool), evictionPolicy(evictionPolicy),
      deallocatePagePort(deallocatePagePort) {}

void DeletePageAction::execute(BufferPoolPageId pageId) {
  std::lock_guard<std::mutex> lock(bufferPool.getSynchronizationMutex());

  // Step 1: Find the resident frame, if this page is currently cached.
  Frame *residentFrame = nullptr;
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (frame.isOccupied() && frame.getPageId() == pageId) {
      residentFrame = &frame;
      break;
    }
  }

  // Step 2: A pinned page is still in use and cannot safely be reclaimed.
  if (residentFrame != nullptr && residentFrame->getPinCount() != 0) {
    throw std::runtime_error("Cannot delete a pinned page");
  }

  // Step 3: Remove the page from replacement tracking before freeing its ID.
  // If allocator metadata rejects the deallocation, restore the policy entry
  // and leave the cached frame intact.
  if (residentFrame != nullptr) {
    evictionPolicy.Remove(pageId);
  }

  try {
    deallocatePagePort.deallocatePage(pageId);
  } catch (...) {
    const std::exception_ptr originalError = std::current_exception();
    if (residentFrame != nullptr) {
      try {
        evictionPolicy.RecordAccess(pageId);
        evictionPolicy.SetEvictable(pageId, true);
      } catch (...) {
        // Preserve the deallocation error if replacement-policy rollback fails.
      }
    }
    std::rethrow_exception(originalError);
  }

  // Step 4: The allocator now owns the free ID; invalidate the cached copy so
  // it cannot later be flushed or fetched as the old page.
  if (residentFrame != nullptr) {
    residentFrame->setPage(BufferPoolPage{});
    residentFrame->setPageId(0);
    residentFrame->setPinCountForDelete(0);
    residentFrame->setDirty(false);
    residentFrame->setOccupied(false);
  }
}
