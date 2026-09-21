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
      unpinPage(bufferPool), flushPage(bufferPool, writePageAdapter),
      newPage(bufferPool, allocatePageAdapter, *evictionPolicy),
      flushAllPages(bufferPool, writePageAdapter),

      // Tuple Service
      pageAdapter(readPage, writePage), createTuple(pageAdapter),
      getTuple(pageAdapter), deleteTuple(pageAdapter), createPage(pageAdapter) {
}

// Disk Manager
// Free Page Manager
// Buffer Pool Manager
// Tuple Service
CreateTupleAction &Container::createTupleAction() { return createTuple; }
GetTupleAction &Container::getTupleAction() { return getTuple; }
DeleteTupleAction &Container::deleteTupleAction() { return deleteTuple; }
CreatePageAction &Container::createPageAction() { return createPage; }