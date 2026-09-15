#include <iostream>
#include <string>

#include "DiskManager/adapter/FileDiskManagerAdapter.h"
#include "PageService/actions/CreatePageAction.h"
#include "PageService/actions/DeleteTupleAction.h"
#include "PageService/actions/GetTupleAction.h"
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
  // 2. CREATE 5 TUPLES
  // --------------------------------------------------

  std::cout << "[2] CREATE 5 TUPLES\n";

  const std::string tuple1 = "Tuple One";
  const std::string tuple2 = "Tuple Two";
  const std::string tuple3 = "Tuple Three";
  const std::string tuple4 = "Tuple Four";
  const std::string tuple5 = "Tuple Five";

  const uint16_t slotId1 = container.createTupleAction().execute(
      pageId, tuple1.data(), static_cast<uint16_t>(tuple1.size()));

  const uint16_t slotId2 = container.createTupleAction().execute(
      pageId, tuple2.data(), static_cast<uint16_t>(tuple2.size()));

  const uint16_t slotId3 = container.createTupleAction().execute(
      pageId, tuple3.data(), static_cast<uint16_t>(tuple3.size()));

  const uint16_t slotId4 = container.createTupleAction().execute(
      pageId, tuple4.data(), static_cast<uint16_t>(tuple4.size()));

  const uint16_t slotId5 = container.createTupleAction().execute(
      pageId, tuple5.data(), static_cast<uint16_t>(tuple5.size()));

  std::cout << "Created tuple 1 -> Slot ID: " << slotId1 << "\n";
  std::cout << "Created tuple 2 -> Slot ID: " << slotId2 << "\n";
  std::cout << "Created tuple 3 -> Slot ID: " << slotId3 << "\n";
  std::cout << "Created tuple 4 -> Slot ID: " << slotId4 << "\n";
  std::cout << "Created tuple 5 -> Slot ID: " << slotId5 << "\n\n";

  // --------------------------------------------------
  // 3. GET TUPLE 2
  // --------------------------------------------------

  std::cout << "[3] GET TUPLE 2\n";

  char tupleData2[Page::PAGE_SIZE]{};

  container.getTupleAction().execute(pageId, slotId2, tupleData2);

  std::string result2(tupleData2, tuple2.size());

  std::cout << "Tuple 2: " << result2 << "\n\n";

  // --------------------------------------------------
  // 4. GET TUPLE 4
  // --------------------------------------------------

  std::cout << "[4] GET TUPLE 4\n";

  char tupleData4[Page::PAGE_SIZE]{};

  container.getTupleAction().execute(pageId, slotId4, tupleData4);

  std::string result4(tupleData4, tuple4.size());

  std::cout << "Tuple 4: " << result4 << "\n\n";

  // --------------------------------------------------
  // 5. DELETE TUPLE 2
  // --------------------------------------------------

  std::cout << "[5] DELETE TUPLE 2\n";

  container.deleteTupleAction().execute(pageId, slotId2);

  std::cout << "Tuple 2 deleted.\n\n";

  // --------------------------------------------------
  // 6. TRY TO GET DELETED TUPLE
  // --------------------------------------------------

  std::cout << "[6] GET DELETED TUPLE 2\n";

  bool deletedTupleBlocked = false;

  try {

    char deletedTupleData[Page::PAGE_SIZE]{};

    container.getTupleAction().execute(pageId, slotId2, deletedTupleData);

    std::cout << "ERROR: Deleted tuple was still readable.\n";

  } catch (const std::exception &e) {

    deletedTupleBlocked = true;

    std::cout << "Correctly failed to get deleted tuple.\n";
    std::cout << "Reason: " << e.what() << "\n";
  }

  std::cout << "\n";

  // --------------------------------------------------
  // 7. VERIFY OTHER TUPLES STILL EXIST
  // --------------------------------------------------

  std::cout << "[7] VERIFY OTHER TUPLES\n";

  char tupleData1[Page::PAGE_SIZE]{};

  container.getTupleAction().execute(pageId, slotId1, tupleData1);

  std::string result1(tupleData1, tuple1.size());

  std::cout << "Tuple 1: " << result1 << "\n";

  char tupleData3[Page::PAGE_SIZE]{};

  container.getTupleAction().execute(pageId, slotId3, tupleData3);

  std::string result3(tupleData3, tuple3.size());

  std::cout << "Tuple 3: " << result3 << "\n";

  char tupleData5[Page::PAGE_SIZE]{};

  container.getTupleAction().execute(pageId, slotId5, tupleData5);

  std::string result5(tupleData5, tuple5.size());

  std::cout << "Tuple 5: " << result5 << "\n\n";

  // --------------------------------------------------
  // FINAL RESULT
  // --------------------------------------------------

  const bool success = result1 == tuple1 && result2 == tuple2 &&
                       result3 == tuple3 && result4 == tuple4 &&
                       result5 == tuple5 && deletedTupleBlocked;

  std::cout << "=========================\n";
  std::cout << "       FINAL RESULT\n";
  std::cout << "=========================\n";

  if (success) {
    std::cout << "ALL TUPLE TESTS PASSED\n";
  } else {
    std::cout << "TUPLE TESTS FAILED\n";
  }

  return success ? 0 : 1;
}