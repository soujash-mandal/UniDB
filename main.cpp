#include "CatalogService/actions/CreateRootCatalogPageAction.h"
#include "CatalogService/adapter/BufferPoolNewPageAdapter.h"
#include "CatalogService/adapter/BufferPoolUnpinPageAdapter.h"
#include "CatalogService/adapter/BufferPoolWritePageAdapter.h"
#include "DatabaseStartup.h"
#include "DiskManager/adapter/FileDiskManagerAdapter.h"
#include "RdbMetadataService/actions/CreateRdbMetadataPageAction.h"
#include "RdbMetadataService/actions/UpdateRdbMetadataPageAction.h"
#include "RdbMetadataService/adapter/BufferPoolFetchPageAdapter.h"
#include "RdbMetadataService/adapter/BufferPoolNewPageAdapter.h"
#include "RdbMetadataService/adapter/BufferPoolUnpinPageAdapter.h"
#include "RdbMetadataService/adapter/BufferPoolWritePageAdapter.h"
#include "RdbService/actions/InitializeRdbAction.h"
#include "RdbService/adapter/CreateRdbMetadataPageAdapter.h"
#include "RdbService/adapter/CreateRootCatalogPageAdapter.h"
#include "RdbService/adapter/UpdateRdbMetadataPageAdapter.h"
#include "container/container.h"

#include <exception>
#include <iostream>
#include <string>

int main() {
  const std::string databaseFile = "database.db";

  try {
    // Check before FileDiskManagerAdapter creates a missing database file.
    const bool databaseAlreadyExists =
        DatabaseStartup::databaseExists(databaseFile);
    FileDiskManagerAdapter diskManager(databaseFile);
    Container container(diskManager, 2, EvictionPolicyType::FIFO);

    BufferPoolFetchPageAdapter metadataFetchPageAdapter(
        container.fetchPageAction());
    BufferPoolNewPageAdapter metadataNewPageAdapter(container.newPageAction());
    BufferPoolUnpinPageAdapter metadataUnpinPageAdapter(
        container.unpinPageAction());
    BufferPoolWritePageAdapter metadataWritePageAdapter(
        container.writePageAction());

    CatalogBufferPoolNewPageAdapter catalogNewPageAdapter(
        container.newPageAction());
    CatalogBufferPoolUnpinPageAdapter catalogUnpinPageAdapter(
        container.unpinPageAction());
    CatalogBufferPoolWritePageAdapter catalogWritePageAdapter(
        container.writePageAction());

    CreateRdbMetadataPageAction createRdbMetadataPageAction(
        metadataNewPageAdapter, metadataWritePageAdapter,
        metadataUnpinPageAdapter);
    UpdateRdbMetadataPageAction updateRdbMetadataPageAction(
        metadataFetchPageAdapter, metadataWritePageAdapter,
        metadataUnpinPageAdapter);
    CreateRootCatalogPageAction createRootCatalogPageAction(
        catalogNewPageAdapter, catalogWritePageAdapter,
        catalogUnpinPageAdapter);

    CreateRdbMetadataPageAdapter createRdbMetadataPageAdapter(
        createRdbMetadataPageAction);
    CreateRootCatalogPageAdapter createRootCatalogPageAdapter(
        createRootCatalogPageAction);
    UpdateRdbMetadataPageAdapter updateRdbMetadataPageAdapter(
        updateRdbMetadataPageAction);

    InitializeRdbAction initializeRdbAction(
        createRdbMetadataPageAdapter,
        createRootCatalogPageAdapter,
        updateRdbMetadataPageAdapter);
    DatabaseStartup databaseStartup(container.createFpmPageAction(),
                                    initializeRdbAction,
                                    container.flushPageAction());
    databaseStartup.execute(databaseAlreadyExists);

    if (databaseAlreadyExists) {
      std::cout << "Existing UniDB database found; skipped initialization.\n";
    } else {
      std::cout << "Initialized new UniDB database.\n";
    }
  } catch (const std::exception &error) {
    std::cerr << "UniDB startup failed: " << error.what() << '\n';
    return 1;
  }

  return 0;
}
