#include "../../actions/SelectAllRowsAction.h"
#include "../RdbActionTestSupport.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace {
class UnexpectedSpaceMapPort final : public SpaceMapPort {
public:
  uint32_t createSpaceMapPage() override { throw std::logic_error("unused"); }
  RdbFindPageWithSpaceResult
  findPageWithSpace(uint32_t, uint32_t) override {
    throw std::logic_error("unused");
  }
  std::vector<uint32_t> getTablePages(uint32_t) override {
    ADD_FAILURE() << "SpaceMap should not be queried without a root page";
    return {};
  }
};

class UnexpectedTuplePagePort final : public TuplePagePort {
public:
  RdbCreatedTuplePage createTuplePage() override {
    throw std::logic_error("unused");
  }
  uint32_t createTuple(uint32_t, std::vector<char>) override {
    throw std::logic_error("unused");
  }
  std::vector<char> getTuple(uint32_t, uint32_t) override {
    throw std::logic_error("unused");
  }
  std::vector<std::vector<char>> getAllTuples(uint32_t) override {
    ADD_FAILURE() << "Tuple pages should not be queried without a root page";
    return {};
  }
};
} // namespace

TEST(SelectAllRowsActionTest, ReturnsEmptyWhenTableHasNoSpaceMapRoot) {
  FakeRdbMetadataPort metadata;
  FakeCatalogTablePort catalog;
  catalog.storedTable.name = "empty_table";
  catalog.storedTable.firstSpaceMapPageId = UINT32_MAX;
  UnexpectedSpaceMapPort spaceMap;
  UnexpectedTuplePagePort tuplePages;
  SelectAllRowsAction action(metadata, catalog, spaceMap, tuplePages);

  EXPECT_TRUE(action.execute("empty_table").empty());
}
