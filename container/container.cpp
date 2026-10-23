#include "container.h"

#include "../EvictionPolicy/EvictionPolicyFactory.h"

#include <utility>

Container::Container(std::string fileName, uint32_t bufferPoolSize,
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
      writePage(bufferPool), discardPage(bufferPool, *evictionPolicy),
      discardPageAdapter(discardPage),
      freePage(readFreePageMetadataAdapter, writeFreePageMetadataAdapter,
               discardPageAdapter),
      metadataFetchPageAdapter(fetchPage),
      metadataNewPageAdapter(newPage), metadataUnpinPageAdapter(unpinPage),
      metadataWritePageAdapter(writePage),
      allocateNextTableId(metadataFetchPageAdapter, metadataWritePageAdapter,
                          metadataUnpinPageAdapter),
      catalogFetchPageAdapter(fetchPage), catalogNewPageAdapter(newPage),
      catalogUnpinPageAdapter(unpinPage), catalogWritePageAdapter(writePage),
      createTable(catalogFetchPageAdapter, catalogNewPageAdapter,
                  catalogWritePageAdapter, catalogUnpinPageAdapter),
      getTableByName(catalogFetchPageAdapter, catalogUnpinPageAdapter),
      updateTable(catalogFetchPageAdapter, catalogWritePageAdapter,
                  catalogUnpinPageAdapter),
      dropTable(catalogFetchPageAdapter, catalogWritePageAdapter,
                catalogUnpinPageAdapter),
      tupleFetchPageAdapter(fetchPage), tupleNewPageAdapter(newPage),
      tupleUnpinPageAdapter(unpinPage), tupleWritePageAdapter(writePage),
      tupleFreePageAdapter(freePage),
      insertTupleIntoTable(tupleFetchPageAdapter, tupleNewPageAdapter,
                           tupleWritePageAdapter, tupleUnpinPageAdapter),
      scanTupleTable(tupleFetchPageAdapter, tupleUnpinPageAdapter),
      scanTupleTableWithLocations(tupleFetchPageAdapter, tupleUnpinPageAdapter),
      dropTupleTable(tupleFetchPageAdapter, tupleUnpinPageAdapter,
                     tupleFreePageAdapter),
      deleteTuple(tupleFetchPageAdapter, tupleWritePageAdapter,
                  tupleUnpinPageAdapter),
      createRdbMetadataPage(metadataNewPageAdapter, metadataWritePageAdapter,
                            metadataUnpinPageAdapter),
      updateRdbMetadataPage(metadataFetchPageAdapter, metadataWritePageAdapter,
                            metadataUnpinPageAdapter),
      createRootCatalogPage(catalogNewPageAdapter, catalogWritePageAdapter,
                            catalogUnpinPageAdapter),
      createRdbMetadataPageAdapter(createRdbMetadataPage),
      createRootCatalogPageAdapter(createRootCatalogPage),
      updateRdbMetadataPageAdapter(updateRdbMetadataPage),
      initializeRdb(createRdbMetadataPageAdapter, createRootCatalogPageAdapter,
                    updateRdbMetadataPageAdapter),
      getCatalogRootId(metadataFetchPageAdapter, metadataUnpinPageAdapter),
      rdbCreateTableAdapter(getCatalogRootId, allocateNextTableId, createTable),
      rdbDropTableAdapter(getCatalogRootId, getTableByName, dropTable,
                          dropTupleTable),
      insertRowAdapter(getCatalogRootId, getTableByName, updateTable,
                       insertTupleIntoTable),
      selectRowsAdapter(getCatalogRootId, getTableByName, scanTupleTable),
      getTableDefinitionAdapter(getCatalogRootId, getTableByName),
      deleteRowsAdapter(getCatalogRootId, getTableByName,
                        scanTupleTableWithLocations, deleteTuple),
      rdbCreateTable(rdbCreateTableAdapter), rdbDropTable(rdbDropTableAdapter),
      insertRow(insertRowAdapter), selectRows(selectRowsAdapter),
      getTableDefinition(getTableDefinitionAdapter),
      deleteRows(deleteRowsAdapter) {
  databaseFileName = std::move(fileName);
  diskManager.fileName = databaseFileName;
}

void Container::close() {
  for (uint32_t frameId = 0; frameId < bufferPool.size(); ++frameId) {
    Frame &frame = bufferPool.getFrame(frameId);
    if (frame.isOccupied() && frame.isDirty()) {
      flushPage.execute(frame.getPageId());
    }
  }
}
