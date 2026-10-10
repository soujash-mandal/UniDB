#include "DatabaseStartup.h"
#include "container/container.h"

#include <exception>
#include <iostream>

int main() {
  try {
    Container container("database.db", 2, EvictionPolicyType::FIFO);

    const bool initializedNewDatabase = DatabaseStartup::execute(container);
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
