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

TEST(SpanMapping, SingleFallbackPreservesCanvasAndFitsFirstMonitor) {
  const std::vector<platf::span_rect_t> client {{0, 0, 2560, 1440}, {2560, 0, 1920, 1080}};
  const auto canvas = platf::span_canvas_bounds(client);
  EXPECT_EQ(canvas.width, 4480);
  EXPECT_EQ(canvas.height, 1440);
  const platf::span_rect_t source {0, 0, 2560, 1440};
  const auto dest = platf::span_fit_rect(source, client.front());
  EXPECT_EQ(dest.x, 0);
  EXPECT_EQ(dest.y, 0);
  EXPECT_EQ(dest.width, 2560);
  EXPECT_EQ(dest.height, 1440);
  const auto point = platf::span_map_point({{dest, source}}, 1280, 720);
  EXPECT_FLOAT_EQ(point.first, 1280);
  EXPECT_FLOAT_EQ(point.second, 720);
}

TEST(SpanMapping, SmallerFirstMonitorFitsEntireHostImage) {
  const auto dest = platf::span_fit_rect({0, 0, 2560, 1440}, {0, 0, 1920, 1080});
  EXPECT_EQ(dest.width, 1920);
  EXPECT_EQ(dest.height, 1080);
  EXPECT_EQ(dest.x, 0);
  EXPECT_EQ(dest.y, 0);
}

TEST(SpanMapping, DifferentAspectRatioAddsLetterboxing) {
  const auto dest = platf::span_fit_rect({0, 0, 2560, 1440}, {0, 0, 1920, 1200});
  EXPECT_EQ(dest.width, 1920);
  EXPECT_EQ(dest.height, 1080);
  EXPECT_EQ(dest.y, 60);
}
