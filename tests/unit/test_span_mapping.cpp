/**
 * @file tests/unit/test_span_mapping.cpp
 * @brief Regression tests for spanned display mouse mapping.
 */
#include "src/platform/common.h"

#include <gtest/gtest.h>

TEST(SpanMapping, AdjacentHorizontalEdgeUsesRightDisplay) {
  const std::vector<platf::span_map_entry_t> map {
    {{0, 0, 2560, 1440}, {0, 0, 2560, 1440}},
    {{2560, 0, 1920, 1080}, {2560, 0, 2560, 1440}},
  };
  const auto point = platf::span_map_point(map, 2560, 540);
  EXPECT_FLOAT_EQ(point.first, 2560);
  EXPECT_FLOAT_EQ(point.second, 720);
}

TEST(SpanMapping, AdjacentVerticalEdgeUsesBottomDisplay) {
  const std::vector<platf::span_map_entry_t> map {
    {{0, 0, 100, 100}, {0, 0, 100, 100}},
    {{0, 100, 100, 100}, {100, 0, 100, 100}},
  };
  const auto point = platf::span_map_point(map, 50, 100);
  EXPECT_FLOAT_EQ(point.first, 150);
  EXPECT_FLOAT_EQ(point.second, 0);
}

TEST(SpanMapping, EmptyMapPreservesCoordinates) {
  const auto point = platf::span_map_point({}, 42, 19);
  EXPECT_FLOAT_EQ(point.first, 42);
  EXPECT_FLOAT_EQ(point.second, 19);
}
