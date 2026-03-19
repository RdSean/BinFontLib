#include "PaperS3EDCApp.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <M5Unified.h>
#include <SD.h>
#include <SPI.h>

#include "PaperS3EDCFontManager.h"
#include "PaperS3EDCModel.h"
#include "PaperS3EDCRenderer.h"

namespace papers3edc {
namespace {
constexpr uint32_t kTouchDebounceMs = 180;
constexpr uint32_t kStatusRefreshMs = 2000;
constexpr int kPadding = 18;
constexpr int kFooterHeight = 62;
constexpr int kFooterTop = 540 - kFooterHeight;
constexpr int kFooterButtonGap = 10;
constexpr int kFooterButtonCount = 4;
constexpr int kSettingsTop = 154;
constexpr int kSettingsRowHeight = 58;
constexpr const char* kWeekdayCn[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};

class PaperS3EDCController {
public:
    PaperS3EDCController()
        : _fonts(&M5.Display), _renderer(&M5.Display, &_fonts) {}

    void begin();
    void loop();

private:
    void mountStorage();
    void seedFakeState();
    void refreshDateAndPower();
    void refreshCounts();
    void setStatus(const char* line, const char* detail);
    void showPage(AppPage page);
    void handleTouch(int x, int y);
    void handleFooterTouch(int x, int y);

    AppState _state{};
    PaperS3EDCFontManager _fonts;
    PaperS3EDCRenderer _renderer;
    bool _touchActive = false;
    uint32_t _lastTouchMs = 0;
    uint32_t _lastStatusRefreshMs = 0;
};

PaperS3EDCController g_app;

void copyText(char* dest, size_t capacity, const char* src) {
    if (!dest || capacity == 0) return;
    if (!src) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, capacity - 1);
    dest[capacity - 1] = '\0';
}

void PaperS3EDCController::setStatus(const char* line, const char* detail) {
    copyText(_state.statusLine, sizeof(_state.statusLine), line);
    copyText(_state.statusDetail, sizeof(_state.statusDetail), detail);
    Serial.printf("[status] %s | %s\n", _state.statusLine, _state.statusDetail);
}

void PaperS3EDCController::mountStorage() {
    _state.littleFsReady = LittleFS.begin(true);
    SPI.begin(39, 40, 38, 47);
    _state.sdReady = SD.begin(47, SPI, 25000000);
    Serial.printf("[fs] LittleFS=%s SD=%s\n", _state.littleFsReady ? "ready" : "failed", _state.sdReady ? "ready" : "failed");
}

void PaperS3EDCController::seedFakeState() {
    struct SeedTask {
        uint16_t id;
        const char* title;
        uint16_t points;
        TaskStatus status;
    };
    static constexpr SeedTask tasks[] = {
        {101, "晨读二十分钟", 2, TaskStatus::Done},
        {102, "完成数学口算一页", 3, TaskStatus::Pending},
        {103, "练字十五分钟", 2, TaskStatus::Pending},
        {104, "整理书包和书桌", 1, TaskStatus::Pending},
        {105, "晚饭后阅读半小时", 3, TaskStatus::Pending},
    };

    _state.taskCount = sizeof(tasks) / sizeof(tasks[0]);
    for (size_t index = 0; index < _state.taskCount; ++index) {
        _state.tasks[index].id = tasks[index].id;
        copyText(_state.tasks[index].title, sizeof(_state.tasks[index].title), tasks[index].title);
        _state.tasks[index].points = tasks[index].points;
        _state.tasks[index].status = tasks[index].status;
    }
    _state.summary.totalPoints = 42;
    _state.summary.streakDays = 6;
    for (size_t index = 0; index < kContributionCellCount; ++index) {
        _state.contribution[index].level = (index % 4);
    }
    refreshDateAndPower();
    refreshCounts();
    setStatus("本地任务已载入", "准备进入今日页");
}

void PaperS3EDCController::refreshDateAndPower() {
    static constexpr const char* const kFallbackWeekday = "星期五";
    int32_t battery = M5.Power.getBatteryLevel();
    _state.summary.batteryPercent = battery;
    _state.summary.charging = (M5.Power.isCharging() == m5::Power_Class::is_charging_t::is_charging);

    if (M5.Rtc.isEnabled()) {
        m5::rtc_datetime_t dt;
        if (M5.Rtc.getDateTime(&dt)) {
            snprintf(_state.summary.dateTitle, sizeof(_state.summary.dateTitle), "%04d年%02d月%02d日", dt.date.year, dt.date.month, dt.date.date);
            copyText(_state.summary.weekLabel, sizeof(_state.summary.weekLabel), kWeekdayCn[dt.date.weekDay & 7]);
            return;
        }
    }
    copyText(_state.summary.dateTitle, sizeof(_state.summary.dateTitle), "2026年03月20日");
    copyText(_state.summary.weekLabel, sizeof(_state.summary.weekLabel), kFallbackWeekday);
}

void PaperS3EDCController::refreshCounts() {
    _state.summary.totalTasks = _state.taskCount;
    _state.summary.completedTasks = 0;
    _state.summary.dayPoints = 0;
    for (size_t index = 0; index < _state.taskCount; ++index) {
        if (_state.tasks[index].status == TaskStatus::Done) {
            ++_state.summary.completedTasks;
            _state.summary.dayPoints += _state.tasks[index].points;
        }
    }
    _state.fullTaskListVisible = (_state.taskCount <= 5);
}

void PaperS3EDCController::showPage(AppPage page) {
    _state.currentPage = page;
    _renderer.render(_state);
}

void PaperS3EDCController::handleFooterTouch(int x, int y) {
    if (y < kFooterTop + 10) {
        return;
    }
    const int buttonWidth = (M5.Display.width() - kPadding * 2 - kFooterButtonGap * (kFooterButtonCount - 1)) / kFooterButtonCount;
    for (int index = 0; index < kFooterButtonCount; ++index) {
        const int left = kPadding + index * (buttonWidth + kFooterButtonGap);
        const int right = left + buttonWidth;
        if (x >= left && x <= right) {
            switch (index) {
                case 0:
                    showPage(AppPage::Today);
                    return;
                case 1:
                    showPage(AppPage::Progress);
                    return;
                case 2:
                    showPage(AppPage::Screensaver);
                    return;
                case 3:
                    showPage(AppPage::Settings);
                    return;
            }
        }
    }
}

void PaperS3EDCController::handleTouch(int x, int y) {
    if (y >= kFooterTop) {
        handleFooterTouch(x, y);
        return;
    }

    if (_state.currentPage == AppPage::Today) {
        const int listTop = 124;
        const int rowHeight = 68;
        for (size_t index = 0; index < _state.taskCount; ++index) {
            const int top = listTop + (int)index * rowHeight;
            const int bottom = top + 58;
            if (y >= top && y <= bottom) {
                _state.selectedTaskIndex = (int)index;
                if (_state.tasks[index].status != TaskStatus::Done) {
                    _state.armedTaskIndex = (int)index;
                    _state.tasks[index].status = TaskStatus::Armed;
                    setStatus("任务已选中", "请确认完成");
                    showPage(AppPage::Confirm);
                }
                return;
            }
        }
        return;
    }

    if (_state.currentPage == AppPage::Confirm) {
        const int buttonY = 374;
        const int buttonW = (M5.Display.width() - kPadding * 2 - 16) / 2;
        if (y >= buttonY && y <= buttonY + 44) {
            if (x >= kPadding && x <= kPadding + buttonW) {
                if (_state.armedTaskIndex >= 0 && _state.armedTaskIndex < (int)_state.taskCount && _state.tasks[_state.armedTaskIndex].status == TaskStatus::Armed) {
                    _state.tasks[_state.armedTaskIndex].status = TaskStatus::Pending;
                }
                _state.armedTaskIndex = -1;
                setStatus("已取消", "返回今日页");
                refreshCounts();
                showPage(AppPage::Today);
                return;
            }
            const int confirmX = kPadding + buttonW + 16;
            if (x >= confirmX && x <= confirmX + buttonW && _state.armedTaskIndex >= 0 && _state.armedTaskIndex < (int)_state.taskCount) {
                TaskItem& task = _state.tasks[_state.armedTaskIndex];
                task.status = TaskStatus::Done;
                task.dirty = true;
                if (_state.pendingEventCount < kMaxPendingEvents) {
                    PendingEvent& event = _state.pendingEvents[_state.pendingEventCount++];
                    event.taskId = task.id;
                    event.points = task.points;
                    event.completedAt = millis();
                }
                _state.summary.totalPoints += task.points;
                _state.contribution[millis() % kContributionCellCount].level = 3;
                _state.armedTaskIndex = -1;
                refreshCounts();
                setStatus("任务已完成", "积分已累计");
                showPage(AppPage::Today);
                return;
            }
        }
        return;
    }

    if (_state.currentPage == AppPage::Settings) {
        for (int index = 0; index < 3; ++index) {
            const int top = kSettingsTop + index * kSettingsRowHeight;
            if (y >= top && y <= top + 42) {
                _state.focusedSettingsIndex = index;
                if (index == 0) {
                    showPage(AppPage::WifiList);
                } else if (index == 1) {
                    _state.syncState = SyncState::Pending;
                    setStatus("已准备同步", "后续接入云端接口");
                    showPage(AppPage::SyncStatus);
                } else {
                    showPage(AppPage::Today);
                }
                return;
            }
        }
        return;
    }

    if (_state.currentPage == AppPage::WifiList) {
        const int listTop = 96;
        for (size_t index = 0; index < std::min<size_t>(_state.wifiNetworkCount, 6); ++index) {
            const int top = listTop + (int)index * 52;
            if (y >= top && y <= top + 40) {
                _state.focusedWifiIndex = (int)index;
                copyText(_state.config.ssid, sizeof(_state.config.ssid), _state.wifiNetworks[index].ssid);
                setStatus("已选择网络", _state.config.ssid);
                showPage(AppPage::WifiPassword);
                return;
            }
        }
        return;
    }

    if (_state.currentPage == AppPage::SyncStatus) {
        showPage(AppPage::Settings);
        return;
    }
}

void PaperS3EDCController::begin() {
    auto config = M5.config();
    M5.begin(config);
    Serial.begin(115200);
    delay(300);
    _renderer.begin();
    mountStorage();
    _state.fontReady = _fonts.load(_state.littleFsReady, _state.sdReady);
    seedFakeState();

    _state.wifiNetworkCount = 3;
    copyText(_state.wifiNetworks[0].ssid, sizeof(_state.wifiNetworks[0].ssid), "SeanHome-5G");
    _state.wifiNetworks[0].rssi = -46;
    copyText(_state.wifiNetworks[1].ssid, sizeof(_state.wifiNetworks[1].ssid), "StudyRoom");
    _state.wifiNetworks[1].rssi = -59;
    copyText(_state.wifiNetworks[2].ssid, sizeof(_state.wifiNetworks[2].ssid), "PaperS3-Guest");
    _state.wifiNetworks[2].rssi = -71;

    Serial.printf("[boot] littlefs=%d sd=%d font=%d path=%s\n",
                  _state.littleFsReady,
                  _state.sdReady,
                  _state.fontReady,
                  _fonts.loadedPath() ? _fonts.loadedPath() : "(none)");
    showPage(AppPage::Today);
}

void PaperS3EDCController::loop() {
    M5.update();
    const uint32_t now = millis();
    if (now - _lastStatusRefreshMs >= kStatusRefreshMs) {
        _lastStatusRefreshMs = now;
        refreshDateAndPower();
    }

    auto touch = M5.Touch.getDetail();
    const bool pressed = touch.isPressed();
    if (pressed) {
        if (!_touchActive && (now - _lastTouchMs) >= kTouchDebounceMs) {
            _lastTouchMs = now;
            handleTouch(touch.x, touch.y);
        }
        _touchActive = true;
    } else {
        _touchActive = false;
    }
    delay(20);
}

}  // namespace
}  // namespace papers3edc

void PaperS3EDC_setup() {
    papers3edc::g_app.begin();
}

void PaperS3EDC_loop() {
    papers3edc::g_app.loop();
}
