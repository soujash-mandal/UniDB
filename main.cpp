#include <algorithm>
#include <iostream>
#include <string>

#include "DiskManagerService/adapter/FileDiskManagerAdapter.h"
#include "core/Page.h"
#include "container/container.h"

int main() {
  FileDiskManagerAdapter diskManagerAdapter("database.db");
  Container container(diskManagerAdapter);

  // test write and read page
  Page page;
  std::string message = "Hello from UniDB";
  std::copy(message.begin(), message.end(), page.data());
  container.writePageAction().execute(0, page);
  Page loadedPage;
  container.readPageAction().execute(0, loadedPage);
  std::cout << loadedPage.data() << std::endl;

  return 0;
}