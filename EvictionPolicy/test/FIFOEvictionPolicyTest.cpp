#include <gtest/gtest.h>

#include "../FIFOEvictionPolicy.h"

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

TEST(FIFOEvictionPolicyTest, RecordAccessDoesNotDuplicatePage) {
  FIFOEvictionPolicy policy;

  policy.RecordAccess(1);
  policy.RecordAccess(2);
  policy.RecordAccess(1);

  policy.SetEvictable(1, true);
  policy.SetEvictable(2, true);

  // Page 1 was already recorded, so accessing it again
  // must not move it behind page 2.
  EXPECT_EQ(policy.Evict(), 1);
  EXPECT_EQ(policy.Evict(), 2);
}

TEST(FIFOEvictionPolicyTest, NonEvictablePageCanBecomeEvictable) {
  FIFOEvictionPolicy policy;

  policy.RecordAccess(1);
  policy.RecordAccess(2);

  policy.SetEvictable(1, false);
  policy.SetEvictable(2, true);

  EXPECT_EQ(policy.Evict(), 2);

  // Page 1 is still present and can later become evictable.
  policy.SetEvictable(1, true);

  EXPECT_EQ(policy.Evict(), 1);
}

TEST(FIFOEvictionPolicyTest, EvictRemovesPageFromPolicy) {
  FIFOEvictionPolicy policy;

  policy.RecordAccess(1);
  policy.SetEvictable(1, true);

  EXPECT_EQ(policy.Evict(), 1);

  // It should no longer exist in the policy.
  policy.SetEvictable(1, true);

  EXPECT_FALSE(policy.Evict().has_value());
}

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

TEST(FIFOEvictionPolicyTest, RemoveUnknownPageDoesNothing) {
  FIFOEvictionPolicy policy;

  policy.RecordAccess(1);
  policy.SetEvictable(1, true);

  policy.Remove(999);

  EXPECT_EQ(policy.Evict(), 1);
  EXPECT_FALSE(policy.Evict().has_value());
}

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