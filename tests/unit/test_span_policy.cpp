/**
 * @file tests/unit/test_span_policy.cpp
 * @brief Test spanned capture recovery decisions without a GPU.
 */
#include "src/span_policy.h"

#include <gtest/gtest.h>

TEST(SpanPolicy, MatchingSdrOutputsAreSupported) {
  EXPECT_TRUE(platf::span_outputs_supported({{true, true, 0}, {true, true, 0}}));
}

TEST(SpanPolicy, MatchingHdrOutputsAreSupported) {
  EXPECT_TRUE(platf::span_outputs_supported({{true, true, 12}, {true, true, 12}}));
}

TEST(SpanPolicy, MixedColorSpacesAreUnsupported) {
  EXPECT_FALSE(platf::span_outputs_supported({{true, true, 0}, {true, true, 12}}));
}

TEST(SpanPolicy, MissingOrSingleOutputIsUnsupported) {
  EXPECT_FALSE(platf::span_outputs_supported({}));
  EXPECT_FALSE(platf::span_outputs_supported({{true, true, 0}}));
}

TEST(SpanPolicy, RotationOrUnknownColorSpaceIsUnsupported) {
  EXPECT_FALSE(platf::span_outputs_supported({{false, true, 0}, {true, true, 0}}));
  EXPECT_FALSE(platf::span_outputs_supported({{true, true, 0}, {true, false, 0}}));
}

TEST(SpanPolicy, FormatFailureStaysBlockedUntilTopologyChanges) {
  EXPECT_TRUE(platf::span_topology_blocked(L"two-sdr", L"two-sdr"));
  EXPECT_FALSE(platf::span_topology_blocked(L"one-sdr", L"two-sdr"));
  EXPECT_FALSE(platf::span_topology_blocked(L"two-hdr", L"two-sdr"));
  EXPECT_FALSE(platf::span_topology_blocked(L"two-sdr", L""));
}

TEST(SpanPolicy, RejectsTransientRefreshRatesBeforePacing) {
  EXPECT_FALSE(platf::span_refresh_rate_valid(0, 0));
  EXPECT_FALSE(platf::span_refresh_rate_valid(60, 0));
  EXPECT_FALSE(platf::span_refresh_rate_valid(0, 1));
  EXPECT_FALSE(platf::span_refresh_rate_valid(1, 100));
  EXPECT_TRUE(platf::span_refresh_rate_valid(60000, 1001));
  EXPECT_TRUE(platf::span_refresh_rate_valid(144, 1));
}
