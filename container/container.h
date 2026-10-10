#pragma once

#include <cstdint>
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
#include "../BufferPoolManager/domain/BufferPool.h"

#include "../BufferPoolManager/adapter/AllocatePageAdapter.h"
#include "../BufferPoolManager/adapter/ReadPageAdapter.h"
#include "../BufferPoolManager/adapter/WritePageAdapter.h"

#include "../FreePageManager/actions/AllocatePageAction.h"
#include "../FreePageManager/actions/CreateFpmPageAction.h"
#include "../FreePageManager/adapter/ReadFreePageMetadataAdapter.h"
#include "../FreePageManager/adapter/WriteFreePageMetadataAdapter.h"

#include "../RdbMetadataService/actions/CreateRdbMetadataPageAction.h"
#include "../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include "../RdbMetadataService/actions/UpdateRdbMetadataPageAction.h"
#include "../RdbMetadataService/adapter/BufferPoolFetchPageAdapter.h"
#include "../RdbMetadataService/adapter/BufferPoolNewPageAdapter.h"
#include "../RdbMetadataService/adapter/BufferPoolUnpinPageAdapter.h"
#include "../RdbMetadataService/adapter/BufferPoolWritePageAdapter.h"

#include "../CatalogService/actions/CreateRootCatalogPageAction.h"
#include "../CatalogService/adapter/BufferPoolFetchPageAdapter.h"
#include "../CatalogService/adapter/BufferPoolNewPageAdapter.h"
#include "../CatalogService/adapter/BufferPoolUnpinPageAdapter.h"
#include "../CatalogService/adapter/BufferPoolWritePageAdapter.h"

#include "../RdbService/actions/InitializeRdbAction.h"
#include "../RdbService/adapter/CreateRdbMetadataPageAdapter.h"
#include "../RdbService/adapter/CreateRootCatalogPageAdapter.h"
#include "../RdbService/adapter/UpdateRdbMetadataPageAdapter.h"

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
  InitializeRdbAction &initializeRdbAction() { return initializeRdb; }
  GetCatalogRootPageIdAction &getCatalogRootPageIdAction() {
    return getCatalogRootId;
  }

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

  BufferPoolFetchPageAdapter metadataFetchPageAdapter;
  BufferPoolNewPageAdapter metadataNewPageAdapter;
  BufferPoolUnpinPageAdapter metadataUnpinPageAdapter;
  BufferPoolWritePageAdapter metadataWritePageAdapter;

  CatalogBufferPoolFetchPageAdapter catalogFetchPageAdapter;
  CatalogBufferPoolNewPageAdapter catalogNewPageAdapter;
  CatalogBufferPoolUnpinPageAdapter catalogUnpinPageAdapter;
  CatalogBufferPoolWritePageAdapter catalogWritePageAdapter;

  CreateRdbMetadataPageAction createRdbMetadataPage;
  UpdateRdbMetadataPageAction updateRdbMetadataPage;
  CreateRootCatalogPageAction createRootCatalogPage;

  CreateRdbMetadataPageAdapter createRdbMetadataPageAdapter;
  CreateRootCatalogPageAdapter createRootCatalogPageAdapter;
  UpdateRdbMetadataPageAdapter updateRdbMetadataPageAdapter;

  InitializeRdbAction initializeRdb;
  GetCatalogRootPageIdAction getCatalogRootId;
};
