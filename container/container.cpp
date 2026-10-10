#include "container.h"

#include "../EvictionPolicy/EvictionPolicyFactory.h"

Container::Container(DiskPort &diskManager, uint32_t bufferPoolSize,
                     EvictionPolicyType evictionPolicyType)
    : readDiskPage(diskManager), writeDiskPage(diskManager),
      readFreePageMetadataAdapter(readDiskPage),
      writeFreePageMetadataAdapter(writeDiskPage),
      allocatePage(readFreePageMetadataAdapter, writeFreePageMetadataAdapter),
      createFpmPage(writeFreePageMetadataAdapter),
      evictionPolicy(EvictionPolicyFactory::Create(evictionPolicyType)),
      readPageAdapter(readDiskPage), writePageAdapter(writeDiskPage),
      bufferPool(bufferPoolSize), allocatePageAdapter(allocatePage),
      fetchPage(bufferPool, readPageAdapter, writePageAdapter, *evictionPolicy),
      unpinPage(bufferPool, *evictionPolicy),
      flushPage(bufferPool, writePageAdapter),
      newPage(bufferPool, allocatePageAdapter, writePageAdapter,
              *evictionPolicy),
      writePage(bufferPool) {}
