#include <cstring>
#include <iostream>
#include <string>

#include "DiskManagerService/adapter/FileDiskManagerAdapter.h"
#include "container/container.h"
#include "core/Page.h"

int main() {

  std::cout << "=========================\n";
  std::cout << "       UniDB CRUD Test\n";
  std::cout << "=========================\n\n";

  // --------------------------------------------------
  // Dependency Injection
  // --------------------------------------------------

  FileDiskManagerAdapter diskManagerAdapter("database.db");

  Container container(diskManagerAdapter);

  // --------------------------------------------------
  // 1. CREATE
  // --------------------------------------------------

  std::cout << "[1] CREATE TUPLE\n";

  char createData[Page::PAGE_SIZE]{};

  std::string createMessage = "Hello from CREATE";

  std::memcpy(createData, createMessage.data(), createMessage.size());

  container.createTupleAction().execute(0, createData);

  std::cout << "Created tuple: ";
  std::cout.write(createData, createMessage.size());
  std::cout << "\n\n";

  // --------------------------------------------------
  // 2. READ
  // --------------------------------------------------

  std::cout << "[2] READ TUPLE\n";

  Page readPage;

  container.readTupleAction().execute(0, readPage);

  std::cout << "Read tuple: ";
  std::cout.write(readPage.data(), createMessage.size());

  std::cout << "\n\n";

  // --------------------------------------------------
  // VERIFY CREATE + READ
  // --------------------------------------------------

  bool createReadSuccess =
      std::memcmp(readPage.data(), createData, createMessage.size()) == 0;

  if (createReadSuccess) {
    std::cout << "CREATE + READ verification: PASS\n\n";
  } else {
    std::cout << "CREATE + READ verification: FAIL\n\n";
  }

  // --------------------------------------------------
  // 3. UPDATE
  // --------------------------------------------------

  std::cout << "[3] UPDATE TUPLE\n";

  char updateData[Page::PAGE_SIZE]{};

  std::string updateMessage = "Hello from UPDATE";

  std::memcpy(updateData, updateMessage.data(), updateMessage.size());

  container.updateTupleAction().execute(0, updateData);

  std::cout << "Updated tuple: ";
  std::cout.write(updateData, updateMessage.size());

  std::cout << "\n\n";

  // --------------------------------------------------
  // 4. READ AFTER UPDATE
  // --------------------------------------------------

  std::cout << "[4] READ AFTER UPDATE\n";

  Page updatedPage;

  container.readTupleAction().execute(0, updatedPage);

  std::cout << "Tuple after update: ";

  std::cout.write(updatedPage.data(), updateMessage.size());

  std::cout << "\n\n";

  // --------------------------------------------------
  // VERIFY UPDATE
  // --------------------------------------------------

  bool updateSuccess =
      std::memcmp(updatedPage.data(), updateData, updateMessage.size()) == 0;

  if (updateSuccess) {
    std::cout << "UPDATE verification: PASS\n\n";
  } else {
    std::cout << "UPDATE verification: FAIL\n\n";
  }

  // --------------------------------------------------
  // 5. DELETE
  // --------------------------------------------------

  std::cout << "[5] DELETE TUPLE\n";

  container.deleteTupleAction().execute(0);

  std::cout << "Tuple deleted.\n\n";

  // --------------------------------------------------
  // 6. READ AFTER DELETE
  // --------------------------------------------------

  std::cout << "[6] READ AFTER DELETE\n";

  Page deletedPage;

  container.readTupleAction().execute(0, deletedPage);

  bool deleteSuccess = true;

  for (std::size_t i = 0; i < Page::PAGE_SIZE; i++) {

    if (deletedPage.data()[i] != 0) {
      deleteSuccess = false;
      break;
    }
  }

  if (deleteSuccess) {
    std::cout << "Page is empty.\n";
    std::cout << "DELETE verification: PASS\n\n";
  } else {
    std::cout << "Page still contains data.\n";
    std::cout << "DELETE verification: FAIL\n\n";
  }

  // --------------------------------------------------
  // FINAL RESULT
  // --------------------------------------------------

  std::cout << "=========================\n";
  std::cout << "       FINAL RESULT\n";
  std::cout << "=========================\n";

  if (createReadSuccess && updateSuccess && deleteSuccess) {

    std::cout << "ALL CRUD TESTS PASSED\n";

  } else {

    std::cout << "SOME CRUD TESTS FAILED\n";
  }

  return 0;
}