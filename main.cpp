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

  // 1. Create a new page.
  std::cout << "Creating new page...\n";
  auto pageId1 = container.newPageAction().execute();
  std::cout << "Created page ID: " << pageId1 << "\n";

  // 2. Write data into the page.
  std::cout << "Writing data to page...\n";
  BufferPoolPage page1;
  const char *message = "Hello UniDB";
  std::memcpy(page1.data(), message, std::strlen(message) + 1);
  container.writePageAction().execute(pageId1, page1);
  std::cout << "Written: " << page1.data() << "\n";

  // 3. Unpin page.
  std::cout << "Unpinning page...\n";
  container.unpinPageAction().execute(pageId1);

  // 4. Flush page to disk.
  std::cout << "Flushing page...\n";
  container.flushPageAction().execute(pageId1);
  std::cout << "Page flushed to disk.\n";

  // 5. Fetch the page again.
  std::cout << "\nFetching page again...\n";
  auto fetchedPage = container.fetchPageAction().execute(pageId1);
  std::cout << "Fetched page ID: " << pageId1 << "\n";
  std::cout << "Fetched data: " << fetchedPage.data() << "\n";

  // 6. Unpin fetched page.
  container.unpinPageAction().execute(pageId1);
  std::cout << "Page unpinned.\n";

  // 7. Create second page.
  std::cout << "\nCreating second page...\n";
  auto pageId2 = container.newPageAction().execute();
  std::cout << "Created page ID: " << pageId2 << "\n";
  container.unpinPageAction().execute(pageId2);

  // 8. Create third page. The two-frame buffer pool should evict an
  // unpinned page.
  std::cout << "\nCreating third page...\n";
  auto pageId3 = container.newPageAction().execute();
  std::cout << "Created page ID: " << pageId3 << "\n";
  container.unpinPageAction().execute(pageId3);

  // 9. Fetch page 1 again after eviction.
  std::cout << "\nFetching page 1 after eviction...\n";
  auto fetchedAgain = container.fetchPageAction().execute(pageId1);
  std::cout << "Fetched page ID: " << pageId1 << "\n";
  std::cout << "Fetched data: " << fetchedAgain.data() << "\n";
  container.unpinPageAction().execute(pageId1);

  std::cout << "\n===== Test Finished =====\n";
  return 0;
}
