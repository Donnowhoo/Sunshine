/**
 * @file tests/unit/test_native_resolution.cpp
 * @brief Native span resolution planning and recovery transaction tests without display changes.
 */
#include "../tests_common.h"

#include <src/native_resolution.h>

namespace {
  /** @brief Build a pair of active QHD devices, intentionally enumerated right-first. */
  display_device::EnumeratedDeviceList native_devices() {
    display_device::EnumeratedDevice left {};
    left.m_device_id = "left";
    left.m_info = display_device::EnumeratedDevice::Info {};
    left.m_info->m_resolution = {2560, 1440};
    left.m_info->m_primary = true;
    auto right = left;
    right.m_device_id = "right";
    right.m_info->m_primary = false;
    right.m_info->m_origin_point = {2560, 0};
    return {right, left};
  }

  const std::vector<platf::span_rect_t> native_host {{0, 0, 2560, 1440}, {2560, 0, 2560, 1440}};  ///< QHD pair.
  const std::vector<platf::span_rect_t> native_client {{0, 0, 2560, 1440}, {2560, 0, 1920, 1080}};  ///< Mixed client.
}  // namespace

TEST(NativeResolution, MatchesDeviceIdentityInCaptureOrder) {
  const auto plan = display_device::plan_native_resolutions(native_devices(), native_host, native_client);
  ASSERT_TRUE(plan);
  EXPECT_EQ(plan->at("left"), (display_device::Resolution {2560, 1440}));
  EXPECT_EQ(plan->at("right"), (display_device::Resolution {1920, 1080}));
}

TEST(NativeResolution, NormalizesNegativeDesktopOrigins) {
  auto devices = native_devices();
  for (auto &device : devices) {
    device.m_info->m_origin_point.m_x -= 2560;
    device.m_info->m_origin_point.m_y -= 100;
  }
  EXPECT_TRUE(display_device::plan_native_resolutions(devices, native_host, native_client));
}

TEST(NativeResolution, RejectsMissingOrAmbiguousCaptureDevices) {
  auto devices = native_devices();
  devices.front().m_info.reset();
  EXPECT_FALSE(display_device::plan_native_resolutions(devices, native_host, native_client));
  devices = native_devices();
  devices.front().m_info->m_origin_point = {0, 0};
  EXPECT_FALSE(display_device::plan_native_resolutions(devices, native_host, native_client));
  EXPECT_FALSE(display_device::plan_native_resolutions(native_devices(), {}, native_client));
}

TEST(NativeResolution, RejectsInvalidClientSizeAndDuplicateIds) {
  auto client = native_client;
  client[1].width = 0;
  EXPECT_FALSE(display_device::plan_native_resolutions(native_devices(), native_host, client));
  client[1].width = 20000;
  EXPECT_FALSE(display_device::plan_native_resolutions(native_devices(), native_host, client));
  auto devices = native_devices();
  devices.front().m_device_id = "left";
  EXPECT_FALSE(display_device::plan_native_resolutions(devices, native_host, native_client));
}

#ifdef _WIN32
  #include <display_device/windows/settings_manager.h>
  #include <src/platform/windows/native_resolution.h>

namespace {
  using namespace display_device;

  /** @brief In-memory recovery storage with write-failure injection. */
  class native_memory_store: public SettingsPersistenceInterface {
  public:
    std::vector<std::uint8_t> bytes {};  ///< Persisted bytes.
    bool fail_store {false};  ///< Simulated write failure.
    bool corrupt_store {false};  ///< Simulated successful but corrupt write.

    bool store(const std::vector<std::uint8_t> &data) override {
      if (fail_store) {
        return false;
      }
      bytes = corrupt_store ? std::vector<std::uint8_t> {0} : data;
      return true;
    }

    std::optional<std::vector<std::uint8_t>> load() const override {
      return bytes;
    }

    bool clear() override {
      bytes.clear();
      return true;
    }
  };

  /** @brief Fake display API: tests never call Windows display functions. */
  class native_fake_api: public WinDisplayDeviceInterface {
  public:
    DeviceDisplayModeMap modes {{"left", {{2560, 1440}, {144, 1}}}, {"right", {{2560, 1440}, {60, 1}}}};  ///< Desktop state.
    ActiveTopology topology {{"left"}, {"right"}};  ///< Active extended displays.
    int writes {};  ///< Mode change calls.
    bool fail_apply {};  ///< Partial change failure.
    bool ignore_apply {};  ///< Driver silently ignores mode request.
    bool topology_changed {};  ///< Simulated unplug after planning.
    std::shared_ptr<native_memory_store> store {};  ///< Journal checked at mutation time.

    bool isApiAccessAvailable() const override {
      return true;
    }

    EnumeratedDeviceList enumAvailableDevices() const override {
      return native_devices();
    }

    std::string getDisplayName(const std::string &id) const override {
      return id;
    }

    ActiveTopology getCurrentTopology() const override {
      return topology;
    }

    bool isTopologyValid(const ActiveTopology &value) const override {
      return !value.empty();
    }

    bool isTopologyTheSame(const ActiveTopology &a, const ActiveTopology &b) const override {
      return !topology_changed && a == b;
    }

    bool setTopology(const ActiveTopology &value) override {
      topology = value;
      return true;
    }

    DeviceDisplayModeMap getCurrentDisplayModes(const StringSet &) const override {
      return modes;
    }

    bool setDisplayModes(const DeviceDisplayModeMap &value) override {
      EXPECT_FALSE(store->bytes.empty());
      ++writes;
      if (!ignore_apply) {
        modes = value;
      }
      return !fail_apply;
    }

    bool isPrimary(const std::string &id) const override {
      return id == "left";
    }

    bool setAsPrimary(const std::string &) override {
      ADD_FAILURE() << "Native resolution must not change primary display";
      return false;
    }

    HdrStateMap getCurrentHdrStates(const StringSet &) const override {
      return {};
    }

    bool setHdrStates(const HdrStateMap &) override {
      ADD_FAILURE() << "Native resolution must not change HDR";
      return false;
    }
  };

  /** @brief Transaction fixture with no real filesystem or display access. */
  class NativeModeTransaction: public testing::Test {
  protected:
    std::shared_ptr<native_memory_store> storage = std::make_shared<native_memory_store>();  ///< Recovery storage.
    std::shared_ptr<native_fake_api> api = std::make_shared<native_fake_api>();  ///< Fake hardware.
    PersistentState persistence {storage};  ///< Cached recovery state.
    StringMap<Resolution> resolutions {{"left", {2560, 1440}}, {"right", {1920, 1080}}};  ///< Requested modes.

    void SetUp() override {
      api->store = storage;
    }
  };
}  // namespace

TEST_F(NativeModeTransaction, PersistsBeforeApplyingAndRestoresAfterRestart) {
  const auto original = api->modes;
  EXPECT_EQ(apply_native_resolutions(*api, persistence, resolutions), native_mode_result_e::applied);
  EXPECT_EQ(api->modes.at("right").m_resolution, (Resolution {1920, 1080}));
  EXPECT_EQ(api->modes.at("left"), original.at("left"));
  EXPECT_EQ(api->modes.at("right").m_refresh_rate, original.at("right").m_refresh_rate);
  SettingsManager restarted {api, nullptr, std::make_unique<PersistentState>(storage), {}};
  EXPECT_EQ(restarted.revertSettings(), SettingsManagerInterface::RevertResult::Ok);
  EXPECT_EQ(api->modes, original);
  EXPECT_TRUE(storage->bytes.empty());
}

TEST_F(NativeModeTransaction, FailedJournalWriteNeverChangesDisplay) {
  storage->fail_store = true;
  EXPECT_EQ(apply_native_resolutions(*api, persistence, resolutions), native_mode_result_e::unavailable);
  EXPECT_EQ(api->writes, 0);
}

TEST_F(NativeModeTransaction, CorruptJournalNeverChangesDisplay) {
  storage->corrupt_store = true;
  EXPECT_EQ(apply_native_resolutions(*api, persistence, resolutions), native_mode_result_e::restore_required);
  EXPECT_EQ(api->writes, 0);
}

TEST_F(NativeModeTransaction, PartialApplyFailureRetainsOriginalRecoveryModes) {
  const auto original = api->modes;
  api->fail_apply = true;
  EXPECT_EQ(apply_native_resolutions(*api, persistence, resolutions), native_mode_result_e::restore_required);
  ASSERT_TRUE(persistence.getState());
  EXPECT_EQ(persistence.getState()->m_modified.m_original_modes, original);
}

TEST_F(NativeModeTransaction, DriverIgnoringResolutionIsNotReportedAsSuccess) {
  api->ignore_apply = true;
  EXPECT_EQ(apply_native_resolutions(*api, persistence, resolutions), native_mode_result_e::restore_required);
}

TEST_F(NativeModeTransaction, PendingRecoveryIsNeverOverwritten) {
  ASSERT_EQ(apply_native_resolutions(*api, persistence, resolutions), native_mode_result_e::applied);
  const auto journal = storage->bytes;
  resolutions["right"] = {1280, 720};
  EXPECT_EQ(apply_native_resolutions(*api, persistence, resolutions), native_mode_result_e::unavailable);
  EXPECT_EQ(api->writes, 1);
  EXPECT_EQ(storage->bytes, journal);
}

TEST_F(NativeModeTransaction, UnplugDuringPreparationPreventsApply) {
  api->topology_changed = true;
  EXPECT_EQ(apply_native_resolutions(*api, persistence, resolutions), native_mode_result_e::restore_required);
  EXPECT_EQ(api->writes, 0);
}

TEST_F(NativeModeTransaction, AlreadyNativeDoesNotWriteJournalOrDisplay) {
  resolutions["right"] = {2560, 1440};
  EXPECT_EQ(apply_native_resolutions(*api, persistence, resolutions), native_mode_result_e::unchanged);
  EXPECT_EQ(api->writes, 0);
  EXPECT_TRUE(storage->bytes.empty());
}

TEST_F(NativeModeTransaction, RejectsClonedDisplaysAndMissingTargets) {
  api->topology = {{"left", "right"}};
  EXPECT_EQ(apply_native_resolutions(*api, persistence, resolutions), native_mode_result_e::unavailable);
  api->topology = {{"left"}, {"right"}};
  resolutions.erase("right");
  EXPECT_EQ(apply_native_resolutions(*api, persistence, resolutions), native_mode_result_e::unavailable);
  EXPECT_EQ(api->writes, 0);
}
#endif
