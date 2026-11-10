/**
 * @file src/span_policy.h
 * @brief Pure decisions for spanned capture recovery.
 */
#pragma once

#include <string>
#include <vector>

namespace platf {
  /**
   * @brief Capture capabilities of one attached output.
   */
  struct span_output_state_t {
    bool unrotated {};  ///< Whether rotation-free composition is possible.
    bool color_known {};  ///< Whether querying the color space succeeded.
    unsigned int color_space {};  ///< Native color space identifier.
  };

  /**
   * @brief Decide whether a set of outputs can share a capture canvas.
   * @param outputs Attached outputs of one GPU.
   * @return True when at least two outputs have compatible capabilities.
   */
  inline bool span_outputs_supported(const std::vector<span_output_state_t> &outputs) {
    if (outputs.size() < 2) {
      return false;
    }
    for (const auto &output : outputs) {
      if (!output.unrotated || !output.color_known || output.color_space != outputs.front().color_space) {
        return false;
      }
    }
    return true;
  }

  /**
   * @brief Reject transient refresh rates that would break frame pacing arithmetic.
   * @param numerator Refresh rate numerator.
   * @param denominator Refresh rate denominator.
   * @return True for a finite rate of at least one Hz.
   */
  inline bool span_refresh_rate_valid(unsigned int numerator, unsigned int denominator) {
    return denominator != 0 && numerator >= denominator;
  }

  /**
   * @brief Decide whether the current topology is latched to single-output capture.
   * @param current Current topology identity.
   * @param failed Topology that previously produced incompatible frame formats.
   * @return True until the observed topology changes.
   */
  inline bool span_topology_blocked(const std::wstring &current, const std::wstring &failed) {
    return !failed.empty() && current == failed;
  }
}  // namespace platf
