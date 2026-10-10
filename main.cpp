#include "DatabaseStartup.h"
#include "DiskManager/adapter/FileDiskManagerAdapter.h"
#include "container/container.h"

#include <exception>
#include <iostream>

int main() {
  try {
    FileDiskManagerAdapter diskManager;
    diskManager.setFileName("database.db");

    Container container(diskManager, 2, EvictionPolicyType::FIFO);
    DatabaseStartup databaseStartup(container);

    const bool initializedNewDatabase = databaseStartup.execute();
    if (initializedNewDatabase) {
      std::cout << "Initialized new UniDB database.\n";
    } else {
      std::cout << "Validated existing UniDB database.\n";
    }
  } catch (const std::exception &error) {
    std::cerr << "UniDB startup failed: " << error.what() << '\n';
    return 1;
  }

  return 0;
}
