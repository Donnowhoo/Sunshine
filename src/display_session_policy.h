/**
 * @file src/display_session_policy.h
 * @brief Session admission policy for shared physical monitor settings.
 */
#pragma once

#include <cstddef>

namespace display_device {
  /** @brief Caller-locked state covering both pending handshakes and running streams. */
  struct display_session_policy_t {
    std::size_t leases {};  ///< Live reservations.
    bool revert_requested {};  ///< Recovery requested while settings are frozen.

    /** @brief Whether preparation may change the physical display modes. */
    bool can_prepare() const {
      return leases == 0;
    }

    /**
     * @brief Reserve a prepared connection, including its handshake lifetime.
     * @param native Whether the connection may have requested temporary native modes.
     */
    void acquire(bool native) {
      ++leases;
      revert_requested = revert_requested || native;
    }

    /** @brief Release a reservation; return true only when the final reservation ends. */
    bool release() {
      if (!leases) {
        return false;
      }
      return --leases == 0;
    }
  };
}  // namespace display_device
