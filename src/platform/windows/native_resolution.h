/**
 * @file src/platform/windows/native_resolution.h
 * @brief Persist original display modes before applying native span resolutions.
 */
#pragma once

#include <algorithm>
#include <display_device/windows/json.h>
#include <display_device/windows/persistent_state.h>
#include <display_device/windows/settings_utils.h>
#include <display_device/windows/win_display_device_interface.h>

namespace display_device {
  /**
   * @brief Result of a native mode transaction; pending state must be reverted on failure.
   */
  enum class native_mode_result_e {
    unchanged,  ///< Requested resolutions already match.
    applied,  ///< Resolutions applied and verified.
    unavailable,  ///< No changes made because prerequisites were not satisfied.
    restore_required  ///< Original state is persisted; restore before any further changes.
  };

  /**
   * @brief Save and verify recovery data before applying a complete set of native resolutions.
   * @param api Windows display API (mockable for tests).
   * @param persistence Recovery state shared with Sunshine's normal settings manager.
   * @param resolutions Requested resolutions for every active device.
   * @return Transaction result; never overwrites an outstanding recovery record.
   */
  inline native_mode_result_e apply_native_resolutions(WinDisplayDeviceInterface &api, PersistentState &persistence, const StringMap<Resolution> &resolutions) {
    using enum native_mode_result_e;
    if (persistence.getState() || resolutions.size() < 2 || !api.isApiAccessAvailable()) {
      return unavailable;
    }
    const auto topology = api.getCurrentTopology();
    if (!api.isTopologyValid(topology) || std::any_of(topology.begin(), topology.end(), [](const auto &group) {
          return group.size() != 1;
        })) {
      return unavailable;
    }
    const auto ids = win_utils::flattenTopology(topology);
    const auto original = api.getCurrentDisplayModes(ids);
    if (original.size() != ids.size() || resolutions.size() != ids.size()) {
      return unavailable;
    }
    auto desired = original;
    for (const auto &[id, resolution] : resolutions) {
      const auto it = desired.find(id);
      if (it == desired.end() || !resolution.m_width || !resolution.m_height) {
        return unavailable;
      }
      it->second.m_resolution = resolution;
    }
    if (desired == original) {
      return unchanged;
    }
    SingleDisplayConfigState state {};
    state.m_initial.m_topology = topology;
    for (const auto &device : api.enumAvailableDevices()) {
      if (device.m_info && device.m_info->m_primary) {
        state.m_initial.m_primary_devices.insert(device.m_device_id);
      }
    }
    if (state.m_initial.m_primary_devices.empty()) {
      return unavailable;
    }
    state.m_modified.m_topology = topology;
    state.m_modified.m_original_modes = original;
    if (!persistence.persistState(state)) {
      return unavailable;
    }
    // FileSettingsPersistence does not report every stream write failure. Read back
    // the journal before allowing a display mutation.
    const auto stored = persistence.getSettingsPersistenceApi()->load();
    SingleDisplayConfigState verified {};
    if (!stored || !fromJson(std::string(stored->begin(), stored->end()), verified) || verified != state) {
      return restore_required;
    }
    // Recheck after persistence: unplugging a device must not target another layout.
    if (!api.isTopologyTheSame(api.getCurrentTopology(), topology) || api.getCurrentDisplayModes(ids) != original) {
      return restore_required;
    }
    if (!api.setDisplayModes(desired)) {
      return restore_required;
    }
    const auto actual = api.getCurrentDisplayModes(ids);
    if (!api.isTopologyTheSame(api.getCurrentTopology(), topology) || actual.size() != desired.size()) {
      return restore_required;
    }
    for (const auto &[id, mode] : desired) {
      const auto it = actual.find(id);
      if (it == actual.end() || it->second.m_resolution != mode.m_resolution) {
        return restore_required;
      }
    }
    return applied;
  }
}  // namespace display_device
