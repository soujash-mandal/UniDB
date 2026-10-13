#include <gtest/gtest.h>

#include "../FIFOEvictionPolicy.h"

// Scenario 1: When all pages are evictable, FIFO selects pages in original insertion order.
TEST(FIFOEvictionPolicyTest, EvictsInInsertionOrder) {
  FIFOEvictionPolicy policy;
  policy.RecordAccess(1);
  policy.RecordAccess(2);
  policy.RecordAccess(3);
  policy.SetEvictable(1, true);
  policy.SetEvictable(2, true);
  policy.SetEvictable(3, true);

  EXPECT_EQ(policy.Evict(), 1);
  EXPECT_EQ(policy.Evict(), 2);
  EXPECT_EQ(policy.Evict(), 3);
  EXPECT_FALSE(policy.Evict().has_value());
}

// Scenario 2: A non-evictable (pinned) oldest page is skipped rather than selected as the victim.
TEST(FIFOEvictionPolicyTest, DoesNotEvictNonEvictablePage) {
  FIFOEvictionPolicy policy;
  policy.RecordAccess(1);
  policy.RecordAccess(2);
  policy.RecordAccess(3);
  policy.SetEvictable(1, false);
  policy.SetEvictable(2, true);
  policy.SetEvictable(3, true);

  EXPECT_EQ(policy.Evict(), 2);
  EXPECT_EQ(policy.Evict(), 3);
  EXPECT_FALSE(policy.Evict().has_value());
}

// Scenario 3: FIFO skips several pinned pages while preserving the relative order of eligible pages.
TEST(FIFOEvictionPolicyTest, SkipsNonEvictablePagesAndPreservesFIFOOrder) {
  FIFOEvictionPolicy policy;
  policy.RecordAccess(1);
  policy.RecordAccess(2);
  policy.RecordAccess(3);
  policy.RecordAccess(4);
  policy.SetEvictable(1, false);
  policy.SetEvictable(2, true);
  policy.SetEvictable(3, false);
  policy.SetEvictable(4, true);

  EXPECT_EQ(policy.Evict(), 2);
  EXPECT_EQ(policy.Evict(), 4);
  EXPECT_FALSE(policy.Evict().has_value());
}

// Scenario 4: Repeated access to a resident page does not duplicate it or change its FIFO position.
TEST(FIFOEvictionPolicyTest, RecordAccessDoesNotDuplicatePage) {
  FIFOEvictionPolicy policy;
  policy.RecordAccess(1);
  policy.RecordAccess(2);
  policy.RecordAccess(1);
  policy.SetEvictable(1, true);
  policy.SetEvictable(2, true);

  EXPECT_EQ(policy.Evict(), 1);
  EXPECT_EQ(policy.Evict(), 2);
}

// Scenario 5: A previously pinned page can become evictable after its final unpin.
TEST(FIFOEvictionPolicyTest, NonEvictablePageCanBecomeEvictable) {
  FIFOEvictionPolicy policy;
  policy.RecordAccess(1);
  policy.RecordAccess(2);
  policy.SetEvictable(1, false);
  policy.SetEvictable(2, true);

  EXPECT_EQ(policy.Evict(), 2);
  policy.SetEvictable(1, true);
  EXPECT_EQ(policy.Evict(), 1);
}

// Scenario 6: Evict removes the selected page from policy state so it cannot be selected twice.
TEST(FIFOEvictionPolicyTest, EvictRemovesPageFromPolicy) {
  FIFOEvictionPolicy policy;
  policy.RecordAccess(1);
  policy.SetEvictable(1, true);

  EXPECT_EQ(policy.Evict(), 1);
  policy.SetEvictable(1, true);
  EXPECT_FALSE(policy.Evict().has_value());
}

// Scenario 7: Explicit removal deletes both ordering and evictability state for a resident page.
TEST(FIFOEvictionPolicyTest, RemoveRemovesPageCompletely) {
  FIFOEvictionPolicy policy;
  policy.RecordAccess(1);
  policy.RecordAccess(2);
  policy.RecordAccess(3);
  policy.SetEvictable(1, true);
  policy.SetEvictable(2, true);
  policy.SetEvictable(3, true);
  policy.Remove(2);

  EXPECT_EQ(policy.Evict(), 1);
  EXPECT_EQ(policy.Evict(), 3);
  EXPECT_FALSE(policy.Evict().has_value());
}

// Scenario 8: Removing a non-evictable page does not affect other eligible pages.
TEST(FIFOEvictionPolicyTest, RemoveNonEvictablePage) {
  FIFOEvictionPolicy policy;
  policy.RecordAccess(1);
  policy.RecordAccess(2);
  policy.SetEvictable(1, false);
  policy.SetEvictable(2, true);
  policy.Remove(1);

  EXPECT_EQ(policy.Evict(), 2);
  EXPECT_FALSE(policy.Evict().has_value());
}

// Scenario 9: Removing a PageId absent from the policy is a safe no-op.
TEST(FIFOEvictionPolicyTest, RemoveUnknownPageDoesNothing) {
  FIFOEvictionPolicy policy;
  policy.RecordAccess(1);
  policy.SetEvictable(1, true);
  policy.Remove(999);

  EXPECT_EQ(policy.Evict(), 1);
  EXPECT_FALSE(policy.Evict().has_value());
}

// Scenario 10: Eviction returns no victim when every tracked page is pinned/non-evictable.
TEST(FIFOEvictionPolicyTest, CannotEvictWhenNoPagesAreEvictable) {
  FIFOEvictionPolicy policy;
  policy.RecordAccess(1);
  policy.RecordAccess(2);
  policy.RecordAccess(3);
  policy.SetEvictable(1, false);
  policy.SetEvictable(2, false);
  policy.SetEvictable(3, false);

  EXPECT_FALSE(policy.Evict().has_value());
}

// Scenario 11: Changing an eligible page back to non-evictable prevents it from being selected.
TEST(FIFOEvictionPolicyTest, SetEvictableFalsePreventsEviction) {
  FIFOEvictionPolicy policy;
  policy.RecordAccess(1);
  policy.RecordAccess(2);
  policy.SetEvictable(1, true);
  policy.SetEvictable(2, true);
  policy.SetEvictable(1, false);

  EXPECT_EQ(policy.Evict(), 2);
  EXPECT_FALSE(policy.Evict().has_value());
}

// Scenario 12: An evicted page can be recorded again as a new residency without stale policy entries.
TEST(FIFOEvictionPolicyTest, CanRecordPageAgainAfterEviction) {
  FIFOEvictionPolicy policy;
  policy.RecordAccess(1);
  policy.SetEvictable(1, true);
  ASSERT_EQ(policy.Evict(), 1);

  policy.RecordAccess(1);
  policy.SetEvictable(1, true);

  EXPECT_EQ(policy.Evict(), 1);
  EXPECT_FALSE(policy.Evict().has_value());
}

// Scenario 13: Setting an unknown PageId evictable does not create an eviction candidate.
TEST(FIFOEvictionPolicyTest, UnknownPageCannotBecomeEvictable) {
  FIFOEvictionPolicy policy;
  policy.SetEvictable(404, true);

  EXPECT_FALSE(policy.Evict().has_value());
}
