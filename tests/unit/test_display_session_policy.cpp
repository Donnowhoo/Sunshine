/**
 * @file tests/unit/test_display_session_policy.cpp
 * @brief Test shared display reservations without accessing Windows displays.
 */
#include <gtest/gtest.h>
#include <src/display_session_policy.h>

TEST(DisplaySessionPolicy, DualThenSinglePreservesSettingsUntilBothLeave) {
  display_device::display_session_policy_t policy;
  EXPECT_TRUE(policy.can_prepare());
  policy.acquire(true);
  EXPECT_FALSE(policy.can_prepare());
  policy.acquire(false);
  EXPECT_FALSE(policy.release());
  EXPECT_FALSE(policy.can_prepare());
  EXPECT_TRUE(policy.revert_requested);
  EXPECT_TRUE(policy.release());
  EXPECT_TRUE(policy.can_prepare());
}

TEST(DisplaySessionPolicy, SingleThenDualCannotChangeExistingModes) {
  display_device::display_session_policy_t policy;
  policy.acquire(false);
  EXPECT_FALSE(policy.can_prepare());
  policy.acquire(true);
  EXPECT_FALSE(policy.release());
  EXPECT_FALSE(policy.can_prepare());
  EXPECT_TRUE(policy.release());
  EXPECT_TRUE(policy.revert_requested);
}

TEST(DisplaySessionPolicy, AbandonedHandshakeDoesNotRestoreUnderRunningClient) {
  display_device::display_session_policy_t policy;
  policy.acquire(true);
  policy.acquire(false);
  EXPECT_FALSE(policy.release());
  EXPECT_EQ(policy.leases, 1);
  EXPECT_TRUE(policy.release());
  EXPECT_FALSE(policy.release());
  EXPECT_EQ(policy.leases, 0);
}

TEST(DisplaySessionPolicy, PendingHandshakeBlocksNewModePreparation) {
  display_device::display_session_policy_t policy;
  policy.acquire(true);
  EXPECT_FALSE(policy.can_prepare());
  EXPECT_TRUE(policy.release());
  EXPECT_TRUE(policy.can_prepare());
}
