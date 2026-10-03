#pragma once

#include <memory>

#include "../DiskManager/actions/ReadDiskPageAction.h"
#include "../DiskManager/actions/WriteDiskPageAction.h"
#include "../DiskManager/port/DiskPort.h"
#include "../EvictionPolicy/EvictionPolicy.h"
#include "../EvictionPolicy/EvictionPolicyType.h"

#include "../BufferPoolManager/actions/FetchPageAction.h"
#include "../BufferPoolManager/actions/FlushPageAction.h"
#include "../BufferPoolManager/actions/NewPageAction.h"
#include "../BufferPoolManager/actions/UnpinPageAction.h"
#include "../BufferPoolManager/actions/WritePageAction.h"

#include "../BufferPoolManager/adapter/AllocatePageAdapter.h"
#include "../BufferPoolManager/adapter/ReadPageAdapter.h"
#include "../BufferPoolManager/adapter/WritePageAdapter.h"

#include "../FreePageManager/actions/AllocatePageAction.h"
#include "../FreePageManager/actions/InitializeMetadataPageAction.h"
#include "../FreePageManager/adapter/ReadFreePageMetadataAdapter.h"
#include "../FreePageManager/adapter/WriteFreePageMetadataAdapter.h"

// #include "../TupleService/adapter/BufferPoolFetchPageAdapter.h"
// #include "../TupleService/adapter/BufferPoolNewPageAdapter.h"
// #include "../TupleService/adapter/BufferPoolUnpinPageAdapter.h"

// #include "../TupleService/actions/CreateTuplePageAction.h"
// #include "../TupleService/actions/CreateTupleAction.h"
// #include "../TupleService/actions/DeleteTupleAction.h"
// #include "../TupleService/actions/GetTupleAction.h"

// #include "../FreeSpaceMapService/adapter/FSMBufferPoolFetchPageAdapter.h"
// #include "../FreeSpaceMapService/adapter/FSMBufferPoolNewPageAdapter.h"
// #include "../FreeSpaceMapService/adapter/FSMBufferPoolUnpinPageAdapter.h"

// #include "../FreeSpaceMapService/actions/AddPageAction.h"
// #include "../FreeSpaceMapService/actions/FindPageWithSpaceAction.h"
// #include "../FreeSpaceMapService/actions/UpdateFreeSpaceAction.h"

class Container {

public:
  explicit Container(DiskPort &diskManager, uint32_t bufferPoolSize,
                     EvictionPolicyType evictionPolicyType);

  FetchPageAction &fetchPageAction() { return fetchPage; }

  UnpinPageAction &unpinPageAction() { return unpinPage; }

  FlushPageAction &flushPageAction() { return flushPage; }

  NewPageAction &newPageAction() { return newPage; }
  InitializeMetadataPageAction &initializeMetadataPageAction() {
    return initializeMetadataPage;
  }

  WritePageAction &writePageAction() { return writePage; }

  // Tuple Service
  // CreateTupleAction &createTupleAction();
  // GetTupleAction &getTupleAction();
  // DeleteTupleAction &deleteTupleAction();
  // CreateTuplePageAction &CreateTuplePageAction();

  // // Free Space Map Service
  // AddPageAction &addPageAction();
  // FindPageWithSpaceAction &findPageWithSpaceAction();
  // UpdateFreeSpaceAction &updateFreeSpaceAction();

private:
  // Disk Manager
  ReadDiskPageAction readDiskPage;
  WriteDiskPageAction writeDiskPage;

  // Free Page Manager
  ReadFreePageMetadataAdapter readFreePageMetadataAdapter;
  WriteFreePageMetadataAdapter writeFreePageMetadataAdapter;

  AllocatePageAction allocatePage;
  InitializeMetadataPageAction initializeMetadataPage;

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
  WritePageAction writePage;

  // Tuple Service
  //   BufferPoolFetchPageAdapter fetchPageAdapter;
  //   BufferPoolUnpinPageAdapter unpinPageAdapter;
  //   BufferPoolNewPageAdapter newPageAdapter;
  //   CreateTupleAction createTuple;
  //   GetTupleAction getTuple;
  //   DeleteTupleAction deleteTuple;
  //   CreateTuplePageAction createPage;

  //   // Free Space Map Service
  //   FSMBufferPoolFetchPageAdapter fsmFetchPageAdapter;
  //   FSMBufferPoolUnpinPageAdapter fsmUnpinPageAdapter;
  //   FSMBufferPoolNewPageAdapter fsmNewPageAdapter;
  //   AddPageAction addPage;
  //   FindPageWithSpaceAction findPageWithSpace;
  //   UpdateFreeSpaceAction updateFreeSpace;
};