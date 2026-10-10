#include "container.h"

#include "../EvictionPolicy/EvictionPolicyFactory.h"

#include <utility>

Container::Container(std::string fileName, uint32_t bufferPoolSize,
                     EvictionPolicyType evictionPolicyType)
    : databaseFileName(std::move(fileName)), readDiskPage(diskManager),
      writeDiskPage(diskManager),
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
      writePage(bufferPool), metadataFetchPageAdapter(fetchPage),
      metadataNewPageAdapter(newPage), metadataUnpinPageAdapter(unpinPage),
      metadataWritePageAdapter(writePage), catalogFetchPageAdapter(fetchPage),
      catalogNewPageAdapter(newPage), catalogUnpinPageAdapter(unpinPage),
      catalogWritePageAdapter(writePage),
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
      getCatalogRootId(metadataFetchPageAdapter, metadataUnpinPageAdapter) {
  diskManager.fileName = databaseFileName;
}
