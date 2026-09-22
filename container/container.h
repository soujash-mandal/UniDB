#pragma once

// todo: check once if this is even needed or not while testing
#include <memory>

#include "../DiskManager/actions/ReadPageAction.h"
#include "../DiskManager/actions/WritePageAction.h"
#include "../DiskManager/port/DiskPort.h"

#include "../FreePageManager/actions/AllocatePageAction.h"
#include "../FreePageManager/actions/FreePageAction.h"
#include "../FreePageManager/adapter/ReadFreePageMetadataAdapter.h"
#include "../FreePageManager/adapter/WriteFreePageMetadataAdapter.h"

#include "../EvictionPolicy/EvictionPolicy.h"
#include "../EvictionPolicy/EvictionPolicyType.h"

#include "../BufferPoolManager/actions/FetchPageAction.h"
#include "../BufferPoolManager/actions/FlushAllPagesAction.h"
#include "../BufferPoolManager/actions/FlushPageAction.h"
#include "../BufferPoolManager/actions/NewPageAction.h"
#include "../BufferPoolManager/actions/UnpinPageAction.h"
#include "../BufferPoolManager/adapter/AllocatePageAdapter.h"
#include "../BufferPoolManager/adapter/ReadPageAdapter.h"
#include "../BufferPoolManager/adapter/WritePageAdapter.h"

#include "../TupleService/adapter/BufferPoolFetchPageAdapter.h"
#include "../TupleService/adapter/BufferPoolNewPageAdapter.h"
#include "../TupleService/adapter/BufferPoolUnpinPageAdapter.h"

#include "../TupleService/actions/CreatePageAction.h"
#include "../TupleService/actions/CreateTupleAction.h"
#include "../TupleService/actions/DeleteTupleAction.h"
#include "../TupleService/actions/GetTupleAction.h"

class Container {

public:
  explicit Container(DiskPort &diskManager, uint32_t bufferPoolSize,
                     EvictionPolicyType evictionPolicyType);
  // Tuple Service
  CreateTupleAction &createTupleAction();
  GetTupleAction &getTupleAction();
  DeleteTupleAction &deleteTupleAction();
  CreatePageAction &createPageAction();

private:
  // Disk Manager
  ReadPageAction readPage;
  WritePageAction writePage;

  // Free Page Manager
  ReadFreePageMetadataAdapter readFreePageMetadataAdapter;
  WriteFreePageMetadataAdapter writeFreePageMetadataAdapter;
  AllocatePageAction allocatePage;
  FreePageAction freePage;

  // Eviction Policy
  std::unique_ptr<EvictionPolicy> evictionPolicy;

  // Buffer Pool Manager
  ReadPageAdapter readPageAdapter;
  WritePageAdapter writePageAdapter;
  BufferPool bufferPool;
  AllocatePageAdapter allocatePageAdapter;
  FetchPageAction fetchPage;
  UnpinPageAction unpinPage;
  FlushPageAction flushPage;
  NewPageAction newPage;
  FlushAllPagesAction flushAllPages;

  // Tuple Service
  BufferPoolFetchPageAdapter fetchPageAdapter;
  BufferPoolUnpinPageAdapter unpinPageAdapter;
  BufferPoolNewPageAdapter newPageAdapter;
  CreateTupleAction createTuple;
  GetTupleAction getTuple;
  DeleteTupleAction deleteTuple;
  CreatePageAction createPage;
};