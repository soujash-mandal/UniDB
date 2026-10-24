#include "../../actions/SelectAllRowsAction.h"
#include "../../domain/RdbRowCodec.h"
#include "../RdbActionTestSupport.h"

#include <gtest/gtest.h>

#include <map>
#include <stdexcept>
#include <vector>

namespace {
class FakeSpaceMapPort final : public SpaceMapPort {
public:
  uint32_t createSpaceMapPage() override { throw std::logic_error("unused"); }
  RdbFindPageWithSpaceResult
  findPageWithSpace(uint32_t, uint32_t) override {
    throw std::logic_error("unused");
  }
  std::vector<uint32_t> getTablePages(uint32_t rootPageId) override {
    requestedRootPageId = rootPageId;
    return pageIds;
  }

  uint32_t requestedRootPageId = 0;
  std::vector<uint32_t> pageIds{101, 102};
};

class FakeTuplePagePort final : public TuplePagePort {
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
  std::vector<std::vector<char>> getAllTuples(uint32_t pageId) override {
    requestedPageIds.push_back(pageId);
    return tuplesByPage[pageId];
  }

  std::map<uint32_t, std::vector<std::vector<char>>> tuplesByPage;
  std::vector<uint32_t> requestedPageIds;
};
} // namespace

TEST(SelectAllRowsActionTest, ReturnsRowsFromEveryTuplePage) {
  FakeRdbMetadataPort metadata;
  FakeCatalogTablePort catalog;
  catalog.storedTable.tableId = 9;
  catalog.storedTable.name = "users";
  catalog.storedTable.firstSpaceMapPageId = 70;
  catalog.storedTable.columns = {
      {"id", RdbDataType::INT, 4, false},
      {"name", RdbDataType::VARCHAR, 32, false},
  };
  FakeSpaceMapPort spaceMap;
  FakeTuplePagePort tuplePages;
  const auto first = RdbRowCodec::encode(
      catalog.storedTable, RdbRow{int64_t{1}, std::string("Ada")});
  const auto second = RdbRowCodec::encode(
      catalog.storedTable, RdbRow{int64_t{2}, std::string("Lin")});
  tuplePages.tuplesByPage[101] = {first};
  tuplePages.tuplesByPage[102] = {second};
  SelectAllRowsAction action(metadata, catalog, spaceMap, tuplePages);

  const auto rows = action.execute("users");

  ASSERT_EQ(rows.size(), 2U);
  EXPECT_EQ(rows[0], (RdbRow{int64_t{1}, std::string("Ada")}));
  EXPECT_EQ(rows[1], (RdbRow{int64_t{2}, std::string("Lin")}));
  EXPECT_EQ(spaceMap.requestedRootPageId, 70U);
  EXPECT_EQ(tuplePages.requestedPageIds, (std::vector<uint32_t>{101, 102}));
}
