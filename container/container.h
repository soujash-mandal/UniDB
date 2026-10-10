#pragma once

#include <cstdint>
#include <memory>

#include "../DiskManager/actions/ReadDiskPageAction.h"
#include "../DiskManager/actions/WriteDiskPageAction.h"
#include "../DiskManager/port/DiskPort.h"
#include "../EvictionPolicy/EvictionPolicy.h"
#include "../EvictionPolicy/EvictionPolicyType.h"

#include "../BufferPoolManager/domain/BufferPool.h"
#include "../BufferPoolManager/actions/FetchPageAction.h"
#include "../BufferPoolManager/actions/FlushPageAction.h"
#include "../BufferPoolManager/actions/NewPageAction.h"
#include "../BufferPoolManager/actions/UnpinPageAction.h"
#include "../BufferPoolManager/actions/WritePageAction.h"

#include "../BufferPoolManager/adapter/AllocatePageAdapter.h"
#include "../BufferPoolManager/adapter/ReadPageAdapter.h"
#include "../BufferPoolManager/adapter/WritePageAdapter.h"

#include "../FreePageManager/actions/AllocatePageAction.h"
#include "../FreePageManager/actions/CreateFpmPageAction.h"
#include "../FreePageManager/adapter/ReadFreePageMetadataAdapter.h"
#include "../FreePageManager/adapter/WriteFreePageMetadataAdapter.h"

class Container {
public:
  explicit Container(DiskPort &diskManager, uint32_t bufferPoolSize,
                     EvictionPolicyType evictionPolicyType);

  FetchPageAction &fetchPageAction() { return fetchPage; }
  UnpinPageAction &unpinPageAction() { return unpinPage; }
  FlushPageAction &flushPageAction() { return flushPage; }
  NewPageAction &newPageAction() { return newPage; }
  WritePageAction &writePageAction() { return writePage; }
  CreateFpmPageAction &createFpmPageAction() { return createFpmPage; }

private:
  ReadDiskPageAction readDiskPage;
  WriteDiskPageAction writeDiskPage;

  ReadFreePageMetadataAdapter readFreePageMetadataAdapter;
  WriteFreePageMetadataAdapter writeFreePageMetadataAdapter;

  AllocatePageAction allocatePage;
  CreateFpmPageAction createFpmPage;

  std::unique_ptr<EvictionPolicy> evictionPolicy;

  ReadPageAdapter readPageAdapter;
  WritePageAdapter writePageAdapter;
  BufferPool bufferPool;
  AllocatePageAdapter allocatePageAdapter;

  FetchPageAction fetchPage;
  UnpinPageAction unpinPage;
  FlushPageAction flushPage;
  NewPageAction newPage;
  WritePageAction writePage;
};