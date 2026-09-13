#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "../../core/Page.h"

#include "../../DiskManagerService/actions/ReadPageAction.h"
#include "../../DiskManagerService/actions/WritePageAction.h"
#include "../../DiskManagerService/adapter/FileDiskManagerAdapter.h"

#include "../actions/CreateTupleAction.h"
#include "../actions/DeleteTupleAction.h"
#include "../actions/ReadTupleAction.h"
#include "../actions/UpdateTupleAction.h"
#include "../adapter/DiskManagerPageAdapter.h"

class TupleActionsTest : public ::testing::Test {

protected:
  const std::string databaseFile = "test_database.db";

  // --------------------------------
  // Disk Manager
  // --------------------------------

  FileDiskManagerAdapter diskManagerAdapter{databaseFile};

  ReadPageAction readPage{diskManagerAdapter};

  WritePageAction writePage{diskManagerAdapter};

  // --------------------------------
  // Page Service Adapter
  // --------------------------------

  DiskManagerPageAdapter pageAdapter{readPage, writePage};

  // --------------------------------
  // Page Service Actions
  // --------------------------------

  CreateTupleAction createTuple{pageAdapter};

  ReadTupleAction readTuple{pageAdapter};

  UpdateTupleAction updateTuple{pageAdapter};

  DeleteTupleAction deleteTuple{pageAdapter};

  // --------------------------------
  // Setup
  // --------------------------------

  void SetUp() override {

    std::ofstream file(databaseFile, std::ios::binary | std::ios::trunc);

    file.close();
  }

  // --------------------------------
  // Cleanup
  // --------------------------------

  void TearDown() override { std::remove(databaseFile.c_str()); }

  // --------------------------------
  // Helper: Create Tuple
  // --------------------------------

  void createTestTuple(PageId pageId, const std::string &message) {

    char tupleData[Page::PAGE_SIZE]{};

    std::memcpy(tupleData, message.data(), message.size());

    createTuple.execute(pageId, tupleData);
  }

  // --------------------------------
  // Helper: Read Tuple
  // --------------------------------

  std::string readTestTuple(PageId pageId, std::size_t size) {

    Page page;

    readTuple.execute(pageId, page);

    return std::string(page.data(), size);
  }

  // --------------------------------
  // Helper: Check Empty Page
  // --------------------------------

  bool isPageEmpty(const Page &page) {

    for (std::size_t i = 0; i < Page::PAGE_SIZE; ++i) {

      if (page.data()[i] != '\0') {
        return false;
      }
    }

    return true;
  }
};

// ==================================================
// CREATE
// ==================================================

TEST_F(TupleActionsTest, CreateTuple) {

  const std::string message = "Hello from CREATE";

  createTestTuple(0, message);

  std::string result = readTestTuple(0, message.size());

  EXPECT_EQ(result, message);
}

// ==================================================
// READ
// ==================================================

TEST_F(TupleActionsTest, ReadTuple) {

  const std::string message = "Hello from READ";

  // Create tuple first
  createTestTuple(0, message);

  // Read tuple
  std::string result = readTestTuple(0, message.size());

  EXPECT_EQ(result, message);
}

// ==================================================
// UPDATE
// ==================================================

TEST_F(TupleActionsTest, UpdateTuple) {

  const std::string original = "Hello from CREATE";

  const std::string updated = "Hello from UPDATE";

  // --------------------------------
  // Create original tuple
  // --------------------------------

  createTestTuple(0, original);

  // --------------------------------
  // Update tuple
  // --------------------------------

  char updateData[Page::PAGE_SIZE]{};

  std::memcpy(updateData, updated.data(), updated.size());

  updateTuple.execute(0, updateData);

  // --------------------------------
  // Read updated tuple
  // --------------------------------

  std::string result = readTestTuple(0, updated.size());

  // --------------------------------
  // Verify
  // --------------------------------

  EXPECT_EQ(result, updated);
}

// ==================================================
// DELETE
// ==================================================

TEST_F(TupleActionsTest, DeleteTuple) {

  const std::string message = "Hello from DELETE";

  // --------------------------------
  // Create tuple
  // --------------------------------

  createTestTuple(0, message);

  // --------------------------------
  // Delete tuple
  // --------------------------------

  deleteTuple.execute(0);

  // --------------------------------
  // Read page after delete
  // --------------------------------

  Page page;

  readTuple.execute(0, page);

  // --------------------------------
  // Verify page is empty
  // --------------------------------

  EXPECT_TRUE(isPageEmpty(page));
}

// ==================================================
// COMPLETE CRUD FLOW
// ==================================================

TEST_F(TupleActionsTest, CompleteCrudFlow) {

  const PageId pageId = 0;

  // --------------------------------
  // CREATE
  // --------------------------------

  const std::string created = "Hello from CREATE";

  createTestTuple(pageId, created);

  // --------------------------------
  // READ
  // --------------------------------

  std::string readResult = readTestTuple(pageId, created.size());

  EXPECT_EQ(readResult, created);

  // --------------------------------
  // UPDATE
  // --------------------------------

  const std::string updated = "Hello from UPDATE";

  char updateData[Page::PAGE_SIZE]{};

  std::memcpy(updateData, updated.data(), updated.size());

  updateTuple.execute(pageId, updateData);

  // --------------------------------
  // READ AFTER UPDATE
  // --------------------------------

  std::string updatedResult = readTestTuple(pageId, updated.size());

  EXPECT_EQ(updatedResult, updated);

  // --------------------------------
  // DELETE
  // --------------------------------

  deleteTuple.execute(pageId);

  // --------------------------------
  // READ AFTER DELETE
  // --------------------------------

  Page deletedPage;

  readTuple.execute(pageId, deletedPage);

  // --------------------------------
  // VERIFY DELETE
  // --------------------------------

  EXPECT_TRUE(isPageEmpty(deletedPage));
}