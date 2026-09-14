#include <cstring>
#include <iostream>
#include <string>

#include "DiskManagerService/adapter/FileDiskManagerAdapter.h"
#include "PageService/actions/CreateTupleAction.h"
#include "container/container.h"
#include "core/Page.h"
#include "core/PageHeader.h"
#include "core/Slot.h"

int main() {
  std::cout << "=========================\n";
  std::cout << "   CreateTupleAction Test\n";
  std::cout << "=========================\n\n";

  // --------------------------------------------------
  // Dependency Injection
  // --------------------------------------------------

  FileDiskManagerAdapter diskManagerAdapter("database.db");
  Container container(diskManagerAdapter);

  const PageId pageId = 0;

  // --------------------------------------------------
  // 1. CREATE EMPTY PAGE
  // --------------------------------------------------

  std::cout << "[1] CREATE EMPTY PAGE\n";

  container.createPageAction().execute(0);

  std::cout << "Page created: " << pageId << "\n\n";

  // --------------------------------------------------
  // 2. CREATE TUPLE
  // --------------------------------------------------

  std::cout << "[2] CREATE TUPLE\n";

  std::string message = "Hello from UniDB";

  uint16_t tupleSize = static_cast<uint16_t>(message.size());

  uint16_t slotId =
      container.createTupleAction().execute(pageId, message.data(), tupleSize);

  std::cout << "Tuple created successfully.\n";
  std::cout << "Slot ID: " << slotId << "\n";
  std::cout << "Tuple: " << message << "\n\n";

  // --------------------------------------------------
  // 3. READ RAW PAGE FROM DISK
  // --------------------------------------------------

  std::cout << "[3] READ PAGE FROM DISK\n";

  Page readPage;

  diskManagerAdapter.readPage(pageId, readPage);

  std::cout << "Page read successfully.\n\n";

  // --------------------------------------------------
  // 4. READ PAGE HEADER
  // --------------------------------------------------

  PageHeader readHeader;

  std::memcpy(&readHeader, readPage.data(), sizeof(PageHeader));

  std::cout << "[4] PAGE HEADER\n";

  std::cout << "Page ID: " << readHeader.pageId << "\n";

  std::cout << "Slot count: " << readHeader.slotCount << "\n";

  std::cout << "Free space offset: " << readHeader.freeSpaceOffset << "\n\n";

  // --------------------------------------------------
  // 5. READ SLOT
  // --------------------------------------------------

  Slot readSlot;

  std::memcpy(&readSlot,
              readPage.data() + sizeof(PageHeader) + slotId * sizeof(Slot),
              sizeof(Slot));

  std::cout << "[5] SLOT\n";

  std::cout << "Slot ID: " << slotId << "\n";

  std::cout << "Tuple offset: " << readSlot.offset << "\n";

  std::cout << "Tuple size: " << readSlot.size << "\n\n";

  // --------------------------------------------------
  // 6. READ TUPLE DIRECTLY FROM PAGE
  // --------------------------------------------------

  std::cout << "[6] TUPLE\n";

  std::string readMessage(readPage.data() + readSlot.offset, readSlot.size);

  std::cout << "Tuple from disk: " << readMessage << "\n\n";

  // --------------------------------------------------
  // 7. VERIFY
  // --------------------------------------------------

  bool headerSuccess = readHeader.pageId == pageId && readHeader.slotCount == 1;

  bool slotSuccess = readSlot.size == tupleSize;

  bool tupleSuccess = readMessage == message;

  std::cout << "[7] VERIFICATION\n";

  std::cout << "Header: " << (headerSuccess ? "PASS" : "FAIL") << "\n";

  std::cout << "Slot: " << (slotSuccess ? "PASS" : "FAIL") << "\n";

  std::cout << "Tuple: " << (tupleSuccess ? "PASS" : "FAIL") << "\n\n";

  // --------------------------------------------------
  // FINAL RESULT
  // --------------------------------------------------

  std::cout << "=========================\n";
  std::cout << "       FINAL RESULT\n";
  std::cout << "=========================\n";

  if (headerSuccess && slotSuccess && tupleSuccess) {

    std::cout << "CREATE TUPLE TEST PASSED\n";

  } else {

    std::cout << "CREATE TUPLE TEST FAILED\n";
  }

  return 0;
}
