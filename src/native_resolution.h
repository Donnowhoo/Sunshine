/**
 * @file src/native_resolution.h
 * @brief Planning native per-monitor resolutions without changing the desktop.
 */
#pragma once

#include "platform/common.h"

#include <algorithm>
#include <display_device/types.h>
#include <optional>

namespace display_device {
  /**
   * @brief Match all active Windows displays to the capture layout before planning resolutions.
   * @param devices Enumerated Windows devices, including inactive devices.
   * @param host Compatible capture rectangles, normalized to their bounding box.
   * @param client Requested client rectangles in capture order.
   * @return Device-to-resolution map, or no value for an ambiguous or unsupported layout.
   */
  inline std::optional<StringMap<Resolution>> plan_native_resolutions(EnumeratedDeviceList devices, const std::vector<platf::span_rect_t> &host, const std::vector<platf::span_rect_t> &client) {
    std::erase_if(devices, [](const auto &device) {
      return !device.m_info;
    });
    if (host.size() < 2 || host.size() != client.size() || devices.size() != host.size()) {
      return std::nullopt;
    }
    std::sort(devices.begin(), devices.end(), [](const auto &a, const auto &b) {
      const auto &pa = a.m_info->m_origin_point;
      const auto &pb = b.m_info->m_origin_point;
      return pa.m_x != pb.m_x ? pa.m_x < pb.m_x : pa.m_y < pb.m_y;
    });
    int min_x = devices.front().m_info->m_origin_point.m_x;
    int min_y = devices.front().m_info->m_origin_point.m_y;
    for (const auto &device : devices) {
      min_x = std::min(min_x, device.m_info->m_origin_point.m_x);
      min_y = std::min(min_y, device.m_info->m_origin_point.m_y);
    }
    StringMap<Resolution> result;
    for (std::size_t i = 0; i < devices.size(); ++i) {
      const auto &device = devices[i];
      const auto &info = *device.m_info;
      const auto &rect = host[i];
      const auto &target = client[i];
      if (device.m_device_id.empty() || rect.width <= 0 || rect.height <= 0 || target.width <= 0 || target.height <= 0 || target.width > 16384 || target.height > 16384 || static_cast<long long>(info.m_origin_point.m_x) - min_x != rect.x || static_cast<long long>(info.m_origin_point.m_y) - min_y != rect.y || info.m_resolution.m_width != static_cast<unsigned int>(rect.width) || info.m_resolution.m_height != static_cast<unsigned int>(rect.height)) {
        return std::nullopt;
      }
      if (!result.emplace(device.m_device_id, Resolution {static_cast<unsigned int>(target.width), static_cast<unsigned int>(target.height)}).second) {
        return std::nullopt;
      }
    }
    return result;
  }
}  // namespace display_device
