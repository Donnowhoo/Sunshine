/**
 * @file tests/unit/test_thread_safe.cpp
 * @brief Tests for thread-safe utility types.
 */

// standard includes
#include <cstdint>
#include <type_traits>

// lib includes
#include <gtest/gtest.h>

// local includes
#include "src/thread_safe.h"

static_assert(!std::is_convertible_v<std::uint32_t, safe::queue_t<int>>);

TEST(ThreadSafeShared, StackOwnersInitializeAndReleaseIndependently) {
  int created_a = 0;
  int destroyed_a = 0;
  int created_b = 0;
  int destroyed_b = 0;
  auto a = safe::make_shared<int>([&](int &value) {
    ++created_a;
    value = 1;
    return 0;
  },
                                  [&](int &) {
                                    ++destroyed_a;
                                  });
  auto b = safe::make_shared<int>([&](int &value) {
    ++created_b;
    value = 2;
    return 0;
  },
                                  [&](int &) {
                                    ++destroyed_b;
                                  });
  {
    auto one = a.ref();
    auto two = b.ref();
    auto another = a.ref();
    EXPECT_EQ(*one.get(), 1);
    EXPECT_EQ(*two.get(), 2);
    one.release();
    EXPECT_EQ(destroyed_a, 0);
    EXPECT_EQ(*another.get(), 1);
  }
  EXPECT_EQ(created_a, 1);
  EXPECT_EQ(created_b, 1);
  EXPECT_EQ(destroyed_a, 1);
  EXPECT_EQ(destroyed_b, 1);
}

TEST(ThreadSafeQueue, RejectsNewestItemAtCapacity) {
  safe::queue_t<int> queue {1, safe::queue_t<int>::overflow_policy_e::reject};

  ASSERT_TRUE(queue.raise(1));
  EXPECT_FALSE(queue.raise(2));

  const auto value = queue.pop();
  ASSERT_TRUE(value);
  EXPECT_EQ(*value, 1);
}

TEST(ThreadSafeQueue, DropsQueuedItemsByDefaultAtCapacity) {
  safe::queue_t<int> queue {2};

  ASSERT_TRUE(queue.raise(1));
  ASSERT_TRUE(queue.raise(2));
  ASSERT_TRUE(queue.raise(3));

  const auto value = queue.pop();
  ASSERT_TRUE(value);
  EXPECT_EQ(*value, 3);
}

TEST(ThreadSafeQueue, RejectsItemsAfterStop) {
  safe::queue_t<int> queue;

  queue.stop();

  EXPECT_FALSE(queue.raise(1));
}
