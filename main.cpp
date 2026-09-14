#include <iostream>
#include <string>

#include "DiskManagerService/adapter/FileDiskManagerAdapter.h"
#include "PageService/actions/CreateTupleAction.h"
#include "PageService/actions/ReadTupleAction.h"
#include "container/container.h"
#include "core/Page.h"

int main() {

  std::cout << "=========================\n";
  std::cout << "   Tuple Action Test\n";
  std::cout << "=========================\n\n";

  // --------------------------------------------------
  // Dependency Injection
  // --------------------------------------------------

  FileDiskManagerAdapter diskManagerAdapter("database.db");

  Container container(diskManagerAdapter);

  const PageId pageId = 0;

  // --------------------------------------------------
  // 1. CREATE PAGE
  // --------------------------------------------------

  std::cout << "[1] CREATE PAGE\n";

  container.createPageAction().execute(pageId);

  std::cout << "Page created: " << pageId << "\n\n";

  // --------------------------------------------------
  // 2. CREATE TUPLE
  // --------------------------------------------------

  std::cout << "[2] CREATE TUPLE\n";

  const std::string message = "Hello from UniDB";

  const uint16_t tupleSize = static_cast<uint16_t>(message.size());

  const uint16_t slotId =
      container.createTupleAction().execute(pageId, message.data(), tupleSize);

  std::cout << "Tuple created successfully.\n";
  std::cout << "Slot ID: " << slotId << "\n\n";

  // --------------------------------------------------
  // 3. READ TUPLE
  // --------------------------------------------------

  std::cout << "[3] READ TUPLE\n";

  char tupleData[Page::PAGE_SIZE]{};

  container.readTupleAction().execute(pageId, slotId, tupleData);

  std::string readMessage(tupleData, tupleSize);

  std::cout << "Tuple from disk: " << readMessage << "\n\n";

  // --------------------------------------------------
  // 4. VERIFY
  // --------------------------------------------------

  std::cout << "[4] VERIFICATION\n";

  const bool success = readMessage == message;

  std::cout << "Tuple: " << (success ? "PASS" : "FAIL") << "\n\n";

  // --------------------------------------------------
  // FINAL RESULT
  // --------------------------------------------------

  std::cout << "=========================\n";
  std::cout << "       FINAL RESULT\n";
  std::cout << "=========================\n";

  if (success) {
    std::cout << "CREATE + READ TUPLE TEST PASSED\n";
  } else {
    std::cout << "CREATE + READ TUPLE TEST FAILED\n";
  }

  return success ? 0 : 1;
}
