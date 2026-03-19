#pragma once

#include <Arduino.h>
#include <stdint.h>

namespace papers3edc {

constexpr size_t kMaxTasks = 12;
constexpr size_t kMaxWifiNetworks = 10;
constexpr size_t kMaxPendingEvents = 24;
constexpr size_t kContributionCellCount = 28;

constexpr size_t kTaskTitleCapacity = 96;
constexpr size_t kStatusTextCapacity = 64;
constexpr size_t kSsidCapacity = 33;
constexpr size_t kPasswordCapacity = 65;

enum class AppPage : uint8_t {
    Today = 0,
    Confirm,
    Progress,
    Screensaver,
    Settings,
    WifiList,
    WifiPassword,
    SyncStatus,
};

enum class TaskStatus : uint8_t {
    Pending = 0,
    Armed,
    Done,
};

enum class SyncState : uint8_t {
    Idle = 0,
    Offline,
    Pending,
    Success,
    Failed,
};

struct TaskItem {
    uint16_t id = 0;
    char title[kTaskTitleCapacity] = {0};
    uint16_t points = 0;
    TaskStatus status = TaskStatus::Pending;
    bool dirty = false;
};

struct ContributionCell {
    uint8_t level = 0;
};

struct WifiNetwork {
    char ssid[kSsidCapacity] = {0};
    int32_t rssi = 0;
    bool secure = true;
};

struct PendingEvent {
    uint16_t taskId = 0;
    uint16_t points = 0;
    uint32_t completedAt = 0;
};

struct DeviceConfig {
    bool wifiConfigured = false;
    bool wifiConnected = false;
    char ssid[kSsidCapacity] = {0};
    char password[kPasswordCapacity] = {0};
    char lastSyncLabel[32] = "未同步";
};

struct DailySummary {
    char dateTitle[32] = "2026年03月20日";
    char weekLabel[16] = "星期五";
    uint8_t totalTasks = 0;
    uint8_t completedTasks = 0;
    uint16_t dayPoints = 0;
    uint16_t totalPoints = 0;
    uint8_t streakDays = 0;
    int32_t batteryPercent = -1;
    bool charging = false;
};

struct AppState {
    AppPage currentPage = AppPage::Today;
    int selectedTaskIndex = 0;
    int armedTaskIndex = -1;
    int focusedSettingsIndex = 0;
    int focusedWifiIndex = 0;
    size_t taskCount = 0;
    TaskItem tasks[kMaxTasks]{};
    DailySummary summary{};
    DeviceConfig config{};
    SyncState syncState = SyncState::Idle;
    bool littleFsReady = false;
    bool sdReady = false;
    bool fontReady = false;
    bool fullTaskListVisible = true;
    size_t wifiNetworkCount = 0;
    WifiNetwork wifiNetworks[kMaxWifiNetworks]{};
    char passwordBuffer[kPasswordCapacity] = {0};
    bool passwordUppercase = false;
    size_t pendingEventCount = 0;
    PendingEvent pendingEvents[kMaxPendingEvents]{};
    ContributionCell contribution[kContributionCellCount]{};
    char statusLine[kStatusTextCapacity] = "准备就绪";
    char statusDetail[kStatusTextCapacity] = "本地模式";
};

inline bool isTaskDone(const TaskItem& item) {
    return item.status == TaskStatus::Done;
}

inline bool isTaskArmed(const TaskItem& item) {
    return item.status == TaskStatus::Armed;
}

}  // namespace papers3edc
