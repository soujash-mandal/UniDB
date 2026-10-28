#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "../DiskManager/actions/ReadDiskPageAction.h"
#include "../DiskManager/actions/WriteDiskPageAction.h"
#include "../DiskManager/adapter/FileDiskManagerAdapter.h"
#include "../DiskManager/port/DiskPort.h"
#include "../EvictionPolicy/EvictionPolicy.h"
#include "../EvictionPolicy/EvictionPolicyType.h"

#include "../BufferPoolManager/actions/FetchPageAction.h"
#include "../BufferPoolManager/actions/DeletePageAction.h"
#include "../BufferPoolManager/actions/FlushPageAction.h"
#include "../BufferPoolManager/actions/NewPageAction.h"
#include "../BufferPoolManager/actions/UnpinPageAction.h"
#include "../BufferPoolManager/actions/WritePageAction.h"
#include "../BufferPoolManager/domain/BufferPool.h"

#include "../BufferPoolManager/adapter/AllocatePageAdapter.h"
#include "../BufferPoolManager/adapter/DeallocatePageAdapter.h"
#include "../BufferPoolManager/adapter/ReadPageAdapter.h"
#include "../BufferPoolManager/adapter/WritePageAdapter.h"

#include "../FreePageManager/actions/AllocatePageAction.h"
#include "../FreePageManager/actions/DeallocatePageAction.h"
#include "../FreePageManager/actions/CreateFpmPageAction.h"
#include "../FreePageManager/adapter/ReadFreePageMetadataAdapter.h"
#include "../FreePageManager/adapter/WriteFreePageMetadataAdapter.h"

#include "../RdbMetadataService/actions/AllocateNextTableIdAction.h"
#include "../RdbMetadataService/actions/CreateRdbMetadataPageAction.h"
#include "../RdbMetadataService/actions/GetCatalogRootPageIdAction.h"
#include "../RdbMetadataService/actions/UpdateRdbMetadataPageAction.h"
#include "../RdbMetadataService/adapter/BufferPoolFetchPageAdapter.h"
#include "../RdbMetadataService/adapter/BufferPoolNewPageAdapter.h"
#include "../RdbMetadataService/adapter/BufferPoolUnpinPageAdapter.h"
#include "../RdbMetadataService/adapter/BufferPoolWritePageAdapter.h"

#include "../CatalogService/actions/CreateRootCatalogPageAction.h"
#include "../CatalogService/actions/CreateTableAction.h"
#include "../CatalogService/actions/GetTableByNameAction.h"
#include "../CatalogService/actions/UpdateTableAction.h"
#include "../CatalogService/adapter/BufferPoolFetchPageAdapter.h"
#include "../CatalogService/adapter/BufferPoolNewPageAdapter.h"
#include "../CatalogService/adapter/BufferPoolUnpinPageAdapter.h"
#include "../CatalogService/adapter/BufferPoolWritePageAdapter.h"

#include "../TupleService/adapter/BufferPoolFetchPageAdapter.h"
#include "../TupleService/adapter/BufferPoolNewPageAdapter.h"
#include "../TupleService/adapter/BufferPoolUnpinPageAdapter.h"
#include "../TupleService/adapter/BufferPoolWritePageAdapter.h"

#include "../RdbService/actions/CreateTableAction.h"
#include "../RdbService/actions/GetTableDefinitionAction.h"
#include "../RdbService/actions/InitializeRdbAction.h"
#include "../RdbService/adapter/AllocateNextTableIdAdapter.h"
#include "../RdbService/adapter/CreateCatalogTableAdapter.h"
#include "../RdbService/adapter/CreateRdbMetadataPageAdapter.h"
#include "../RdbService/adapter/CreateRootCatalogPageAdapter.h"
#include "../RdbService/adapter/GetCatalogRootPageIdAdapter.h"
#include "../RdbService/adapter/GetCatalogTableByNameAdapter.h"
#include "../RdbService/adapter/UpdateRdbMetadataPageAdapter.h"
#include "../RdbService/adapter/ReclaimPageAdapter.h"

class Container {
public:
  explicit Container(std::string databaseFileName, uint32_t bufferPoolSize,
                     EvictionPolicyType evictionPolicyType);

  const std::string &getDatabaseFileName() const { return databaseFileName; }

  FetchPageAction &fetchPageAction() { return fetchPage; }
  UnpinPageAction &unpinPageAction() { return unpinPage; }
  FlushPageAction &flushPageAction() { return flushPage; }
  NewPageAction &newPageAction() { return newPage; }
  DeletePageAction &deletePageAction() { return deletePage; }
  RdbService::ReclaimPageAdapter &reclaimPageAdapter() {
    return reclaimPageAdapter;
  }
  WritePageAction &writePageAction() { return writePage; }
  CreateFpmPageAction &createFpmPageAction() { return createFpmPage; }
  InitializeRdbAction &initializeRdbAction() { return initializeRdb; }
  RdbService::CreateTableAction &rdbCreateTableAction() {
    return rdbCreateTable;
  }
  RdbService::GetTableDefinitionAction &rdbGetTableDefinitionAction() {
    return rdbGetTableDefinition;
  }
  GetCatalogRootPageIdAction &getCatalogRootPageIdAction() {
    return getCatalogRootId;
  }
  AllocateNextTableIdAction &allocateNextTableIdAction() {
    return allocateNextTableId;
  }
  CreateTableAction &createTableAction() { return createTable; }
  GetTableByNameAction &getTableByNameAction() { return getTableByName; }
  UpdateTableAction &updateTableAction() { return updateTable; }

private:
  std::string databaseFileName;
  FileDiskManagerAdapter diskManager;

  ReadDiskPageAction readDiskPage;
  WriteDiskPageAction writeDiskPage;

  ReadFreePageMetadataAdapter readFreePageMetadataAdapter;
  WriteFreePageMetadataAdapter writeFreePageMetadataAdapter;

  AllocatePageAction allocatePage;
  DeallocatePageAction deallocatePage;
  CreateFpmPageAction createFpmPage;

  std::unique_ptr<EvictionPolicy> evictionPolicy;

  ReadPageAdapter readPageAdapter;
  WritePageAdapter writePageAdapter;
  BufferPool bufferPool;
  AllocatePageAdapter allocatePageAdapter;
  DeallocatePageAdapter deallocatePageAdapter;

  FetchPageAction fetchPage;
  UnpinPageAction unpinPage;
  DeletePageAction deletePage;
  RdbService::ReclaimPageAdapter reclaimPageAdapter;
  FlushPageAction flushPage;
  NewPageAction newPage;
  WritePageAction writePage;

  BufferPoolFetchPageAdapter metadataFetchPageAdapter;
  BufferPoolNewPageAdapter metadataNewPageAdapter;
  BufferPoolUnpinPageAdapter metadataUnpinPageAdapter;
  BufferPoolWritePageAdapter metadataWritePageAdapter;
  AllocateNextTableIdAction allocateNextTableId;

  CatalogBufferPoolFetchPageAdapter catalogFetchPageAdapter;
  CatalogBufferPoolNewPageAdapter catalogNewPageAdapter;
  CatalogBufferPoolUnpinPageAdapter catalogUnpinPageAdapter;
  CatalogBufferPoolWritePageAdapter catalogWritePageAdapter;

  CreateTableAction createTable;
  GetTableByNameAction getTableByName;
  UpdateTableAction updateTable;

  TupleBufferPoolFetchPageAdapter tupleFetchPageAdapter;
  TupleBufferPoolNewPageAdapter tupleNewPageAdapter;
  TupleBufferPoolUnpinPageAdapter tupleUnpinPageAdapter;
  TupleBufferPoolWritePageAdapter tupleWritePageAdapter;

  CreateRdbMetadataPageAction createRdbMetadataPage;
  UpdateRdbMetadataPageAction updateRdbMetadataPage;
  CreateRootCatalogPageAction createRootCatalogPage;

  CreateRdbMetadataPageAdapter createRdbMetadataPageAdapter;
  CreateRootCatalogPageAdapter createRootCatalogPageAdapter;
  UpdateRdbMetadataPageAdapter updateRdbMetadataPageAdapter;

  InitializeRdbAction initializeRdb;
  GetCatalogRootPageIdAction getCatalogRootId;

  GetCatalogRootPageIdAdapter getCatalogRootPageIdAdapter;
  AllocateNextTableIdAdapter allocateNextTableIdAdapter;
  CreateCatalogTableAdapter createCatalogTableAdapter;
  GetCatalogTableByNameAdapter getCatalogTableByNameAdapter;
  RdbService::CreateTableAction rdbCreateTable;
  RdbService::GetTableDefinitionAction rdbGetTableDefinition;
};
