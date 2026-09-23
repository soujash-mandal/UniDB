#include "container.h"
#include "../EvictionPolicy/EvictionPolicyFactory.h"

Container::Container(
    // IO
    DiskPort &diskManager,
    // configuration
    uint32_t bufferPoolSize,
    EvictionPolicyType evictionPolicyType

    )
    : // Disk Manager
      readPage(diskManager), writePage(diskManager),

      // Free Page Manager
      readFreePageMetadataAdapter(readPage),
      writeFreePageMetadataAdapter(writePage),
      allocatePage(readFreePageMetadataAdapter, writeFreePageMetadataAdapter),
      freePage(readFreePageMetadataAdapter, writeFreePageMetadataAdapter),

      // Eviction Policy
      evictionPolicy(EvictionPolicyFactory::Create(evictionPolicyType)),

      // Buffer Pool Manager
      readPageAdapter(readPage), writePageAdapter(writePage),
      bufferPool(bufferPoolSize), allocatePageAdapter(allocatePage),

      fetchPage(bufferPool, readPageAdapter, writePageAdapter, *evictionPolicy),
      unpinPage(bufferPool, *evictionPolicy),
      flushPage(bufferPool, writePageAdapter),
      newPage(bufferPool, allocatePageAdapter, writePageAdapter,
              *evictionPolicy),
      flushAllPages(bufferPool, writePageAdapter),

      // Tuple Service
      fetchPageAdapter(fetchPage), unpinPageAdapter(unpinPage),
      newPageAdapter(newPage),

      createTuple(fetchPageAdapter, unpinPageAdapter),
      getTuple(fetchPageAdapter, unpinPageAdapter),
      deleteTuple(fetchPageAdapter, unpinPageAdapter),
      createPage(newPageAdapter),

      // Free Space Map Service
      fsmFetchPageAdapter(fetchPage), fsmUnpinPageAdapter(unpinPage),
      fsmNewPageAdapter(newPage),

      addPage(fsmFetchPageAdapter, fsmUnpinPageAdapter, fsmNewPageAdapter),
      findPageWithSpace(fsmFetchPageAdapter, fsmUnpinPageAdapter),
      updateFreeSpace(fsmFetchPageAdapter, fsmUnpinPageAdapter) {}

// Tuple Service
CreateTupleAction &Container::createTupleAction() { return createTuple; }
GetTupleAction &Container::getTupleAction() { return getTuple; }
DeleteTupleAction &Container::deleteTupleAction() { return deleteTuple; }
CreatePageAction &Container::createPageAction() { return createPage; }

// Free Space Map Service
AddPageAction &Container::addPageAction() { return addPage; }

FindPageWithSpaceAction &Container::findPageWithSpaceAction() {
  return findPageWithSpace;
}

UpdateFreeSpaceAction &Container::updateFreeSpaceAction() {
  return updateFreeSpace;
}
