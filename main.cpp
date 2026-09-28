
#include "DiskManager/adapter/FileDiskManagerAdapter.h"
#include "container/container.h"

#include <cstring>
#include <iostream>

int main() {

  const char *databaseFile = "database.db";

  std::cout << "===== UniDB Buffer Pool Manual Test =====\n\n";

  // Real DiskManager adapter.
  FileDiskManagerAdapter diskManager(databaseFile);

  // Real Container.
  // Buffer pool = 2 frames.
  Container container(diskManager, 2, EvictionPolicyType::FIFO);

  // Initialize metadata.
  container.initializeMetadataPageAction().execute();

  // --------------------------------------------------
  // 1. Create a new page
  // --------------------------------------------------

  std::cout << "Creating new page...\n";

  auto pageId1 = container.newPageAction().execute();

  std::cout << "Created page ID: " << pageId1 << "\n";

  // --------------------------------------------------
  // 2. Write data into the page
  // --------------------------------------------------

  std::cout << "Writing data to page...\n";

  BufferPoolPage page1;

  const char *message = "Hello UniDB";

  std::memcpy(page1.data(), message, std::strlen(message) + 1);

  container.writePageAction().execute(pageId1, page1);

  std::cout << "Written: " << page1.data() << "\n";

  // --------------------------------------------------
  // 3. Unpin page and mark it dirty
  // --------------------------------------------------

  std::cout << "Unpinning page...\n";

  container.unpinPageAction().execute(pageId1, true);

  // --------------------------------------------------
  // 4. Flush page to real disk
  // --------------------------------------------------

  std::cout << "Flushing page...\n";

  container.flushPageAction().execute(pageId1);

  std::cout << "Page flushed to disk.\n";

  // --------------------------------------------------
  // 5. Fetch the page again
  // --------------------------------------------------

  std::cout << "\nFetching page again...\n";

  auto fetchedPage = container.fetchPageAction().execute(pageId1);

  std::cout << "Fetched page ID: " << pageId1 << "\n";
  std::cout << "Fetched data: " << fetchedPage.data() << "\n";

  // --------------------------------------------------
  // 6. Unpin fetched page
  // --------------------------------------------------

  container.unpinPageAction().execute(pageId1, false);

  std::cout << "Page unpinned.\n";

  // --------------------------------------------------
  // 7. Create second page
  // --------------------------------------------------

  std::cout << "\nCreating second page...\n";

  auto pageId2 = container.newPageAction().execute();

  std::cout << "Created page ID: " << pageId2 << "\n";

  container.unpinPageAction().execute(pageId2, false);

  // --------------------------------------------------
  // 8. Create third page
  //
  // Buffer pool has only 2 frames.
  //
  // Page 1 and page 2 are both unpinned,
  // so FIFO eviction should be possible.
  // --------------------------------------------------

  std::cout << "\nCreating third page...\n";

  auto pageId3 = container.newPageAction().execute();

  std::cout << "Created page ID: " << pageId3 << "\n";

  container.unpinPageAction().execute(pageId3, false);

  // --------------------------------------------------
  // 9. Fetch page 1 again
  //
  // Page 1 should have been evicted when page 3
  // was created.
  //
  // Fetch should therefore read page 1 from disk.
  // --------------------------------------------------

  std::cout << "\nFetching page 1 after eviction...\n";

  auto fetchedAgain = container.fetchPageAction().execute(pageId1);

  std::cout << "Fetched page ID: " << pageId1 << "\n";
  std::cout << "Fetched data: " << fetchedAgain.data() << "\n";

  container.unpinPageAction().execute(pageId1, false);

  std::cout << "\n===== Test Finished =====\n";

  return 0;
}
