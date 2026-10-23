#include "../actions/NewPageAction.h"

#include <gtest/gtest.h>

#include <optional>

namespace {
class Allocator final : public AllocatePagePort {
public:
  BufferPoolPageId allocatePage() override { return nextPageId++; }
  BufferPoolPageId nextPageId{42};
};

class Writer final : public WritePagePort {
public:
  void writePage(BufferPoolPageId, BufferPoolPage) override {}
};

class Policy final : public EvictionPolicy {
public:
  void RecordAccess(EvictionPolicyPageId) override {}
  void SetEvictable(EvictionPolicyPageId, bool) override {}
  std::optional<EvictionPolicyPageId> Evict() override { return std::nullopt; }
  void Remove(EvictionPolicyPageId) override {}
};
} // namespace

TEST(NewPageActionTest, ClearsReusedEmptyFrameBeforeReturningNewPage) {
  BufferPool pool(1);
  BufferPoolPage stalePage;
  stalePage.data()[0] = 'x';
  Frame &frame = pool.getFrame(0);
  frame.setPage(stalePage);
  frame.setPageId(7);
  frame.setOccupied(false);

  Allocator allocator;
  Writer writer;
  Policy policy;
  NewPageAction action(pool, allocator, writer, policy);

  EXPECT_EQ(action.execute(), 42U);
  EXPECT_EQ(pool.getFrame(0).getPage().data()[0], '\0');
}
