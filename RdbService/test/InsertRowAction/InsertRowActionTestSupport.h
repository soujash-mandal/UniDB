#pragma once

#include "../../actions/InsertRowAction.h"
#include "../../port/CalculateRequiredSpacePort.h"
#include "../../port/CreateTupleWithFreeSpacePort.h"
#include "../../port/UpdateFreeSpacePort.h"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class FakeInsertRootPort final : public GetCatalogRootPageIdPort {
public:
  uint32_t calls = 0;
  uint32_t rootPageId = 17;
  uint32_t getCatalogRootPageId(uint32_t) override {
    ++calls;
    return rootPageId;
  }
};

class FakeInsertTablePort final : public GetCatalogTableByNamePort {
public:
  uint32_t calls = 0;
  uint32_t requestedRoot = 0;
  std::string requestedName;
  RdbTableDefinition table;
  RdbTableDefinition getTableByName(uint32_t root,
                                    const std::string &name) override {
    ++calls;
    requestedRoot = root;
    requestedName = name;
    return table;
  }
};

class FakeInsertUpdateTablePort final : public UpdateCatalogTablePort {
public:
  uint32_t calls = 0;
  uint32_t root = 0;
  uint32_t spaceMapRoot = 0;
  void updateTable(uint32_t requestedRoot, const RdbTableDefinition &,
                   uint32_t requestedSpaceMapRoot) override {
    ++calls;
    root = requestedRoot;
    spaceMapRoot = requestedSpaceMapRoot;
  }
};

class FakeInsertCreateSpaceMapPort final : public CreateSpaceMapPagePort {
public:
  uint32_t calls = 0;
  uint32_t pageId = 71;
  uint32_t createSpaceMapPage() override {
    ++calls;
    return pageId;
  }
};

class FakeInsertCalculateRequiredSpacePort final
    : public RdbService::CalculateRequiredSpacePort {
public:
  uint32_t calls = 0;
  std::vector<char> requestedBytes;
  uint32_t requiredSpace = 0;
  uint32_t calculateRequiredSpace(const std::vector<char> &tupleData) override {
    ++calls;
    requestedBytes = tupleData;
    if (tupleData.size() > 8176) {
      throw std::invalid_argument("Tuple exceeds the V1 tuple size limit");
    }
    requiredSpace = static_cast<uint32_t>(tupleData.size()) + 8;
    return requiredSpace;
  }
};

class FakeInsertFindSpacePort final : public FindPageWithSpacePort {
public:
  uint32_t calls = 0;
  uint32_t root = 0;
  uint32_t requiredSpace = 0;
  RdbFindPageWithSpaceResult result{81, 91};
  RdbFindPageWithSpaceResult
  findPageWithSpace(uint32_t requestedRoot, uint32_t requestedSpace) override {
    ++calls;
    root = requestedRoot;
    requiredSpace = requestedSpace;
    return result;
  }
};

class FakeInsertTupleWithFreeSpacePort final
    : public RdbService::CreateTupleWithFreeSpacePort {
public:
  uint32_t calls = 0;
  uint32_t pageId = 0;
  uint32_t slotId = 4;
  uint32_t remainingFreeSpace = 1234;
  std::vector<char> bytes;
  RdbService::RdbCreateTupleWithFreeSpaceResult
  createTupleWithFreeSpace(uint32_t requestedPageId,
                           std::vector<char> tupleData) override {
    ++calls;
    pageId = requestedPageId;
    bytes = std::move(tupleData);
    return {slotId, remainingFreeSpace};
  }
};

class FakeInsertUpdateFreeSpacePort final
    : public RdbService::UpdateFreeSpacePort {
public:
  uint32_t calls = 0;
  uint32_t spaceMapPageId = 0;
  uint32_t tuplePageId = 0;
  uint32_t freeSpace = 0;
  void updateFreeSpace(uint32_t requestedSpaceMapPageId,
                       uint32_t requestedTuplePageId,
                       uint32_t requestedFreeSpace) override {
    ++calls;
    spaceMapPageId = requestedSpaceMapPageId;
    tuplePageId = requestedTuplePageId;
    freeSpace = requestedFreeSpace;
  }
};
