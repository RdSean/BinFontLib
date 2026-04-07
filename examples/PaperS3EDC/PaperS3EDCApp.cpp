#include "PaperS3EDCApp.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <M5Unified.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>

#include "PaperS3EDCFontManager.h"
#include "PaperS3EDCLayout.h"
#include "PaperS3EDCModel.h"
#include "PaperS3EDCRenderer.h"
#include "PaperS3EDCStorage.h"
#include "PaperS3EDCSync.h"

namespace papers3edc {
namespace {
constexpr uint32_t kTouchDebounceMs = 180;
constexpr uint32_t kStatusRefreshMs = 2000;
constexpr const char* kWeekdayCn[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
constexpr int kMessageRowHeight = 82;
constexpr int kMessageRowGap = 12;
constexpr int kMessageDetailBodyTop = 140;
constexpr int kMessageDetailFooterHeight = 44;
constexpr int kMessageScrollStep = 84;
constexpr uint32_t kMinPollIntervalSec = 2;
constexpr uint32_t kMaxPollIntervalSec = 30;
constexpr uint32_t kPollIntervalStepSec = 2;
constexpr const char* kRelayDeviceId = "papers3-demo";

class PaperS3EDCController {
public:
    PaperS3EDCController()
        : _fonts(&M5.Display), _renderer(&M5.Display, &_fonts), _storage(&LittleFS) {}

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
    void scanWifiNetworks();
    void handleWifiPasswordTouch(int x, int y);
    void appendPasswordChar(const char* label);
    void removeLastPasswordChar();
    void persistState();
    void persistConfig();
    void connectConfiguredWifi();
    void runManualSync();
    void maybePollMessages(uint32_t now);

    AppState _state{};
    PaperS3EDCFontManager _fonts;
    PaperS3EDCRenderer _renderer;
    PaperS3EDCStorage _storage;
    PaperS3EDCSyncService _sync;
    LayoutMetrics _layout{};
    bool _touchActive = false;
    uint32_t _lastTouchMs = 0;
    uint32_t _lastStatusRefreshMs = 0;
    uint32_t _lastMessagePollMs = 0;
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
    struct SeedMessage {
        const char* id;
        const char* title;
        const char* body;
        const char* sentAt;
        const char* source;
        bool read;
    };
    static constexpr SeedMessage messages[] = {
        {"msg_1001", "钉钉提醒", "今天 19:30 前完成数学口算并拍照提交。", "09:12", "班级群", false},
        {"msg_1002", "家长留言", "放学后记得带回英语练习册，晚上一起检查。", "08:05", "妈妈", true},
        {"msg_1003", "老师通知", "本周五需要穿校服参加升旗，别忘了准备红领巾。", "昨天", "班主任", false},
    };

    _state.taskCount = sizeof(tasks) / sizeof(tasks[0]);
    for (size_t index = 0; index < _state.taskCount; ++index) {
        _state.tasks[index].id = tasks[index].id;
        copyText(_state.tasks[index].title, sizeof(_state.tasks[index].title), tasks[index].title);
        _state.tasks[index].points = tasks[index].points;
        _state.tasks[index].status = tasks[index].status;
    }
    _state.messageCount = sizeof(messages) / sizeof(messages[0]);
    for (size_t index = 0; index < _state.messageCount; ++index) {
        copyText(_state.messages[index].message_id, sizeof(_state.messages[index].message_id), messages[index].id);
        copyText(_state.messages[index].title, sizeof(_state.messages[index].title), messages[index].title);
        copyText(_state.messages[index].body, sizeof(_state.messages[index].body), messages[index].body);
        copyText(_state.messages[index].sent_at, sizeof(_state.messages[index].sent_at), messages[index].sentAt);
        copyText(_state.messages[index].source, sizeof(_state.messages[index].source), messages[index].source);
        _state.messages[index].read = messages[index].read;
    }
    copyText(_state.lastMessageCursor, sizeof(_state.lastMessageCursor), "seed-cursor-001");
    _state.summary.totalPoints = 42;
    _state.summary.streakDays = 6;
    for (size_t index = 0; index < kContributionCellCount; ++index) {
        _state.contribution[index].level = (index % 4);
    }
    refreshDateAndPower();
    refreshCounts();
    setStatus("任务已载入", "进入今日");
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

void PaperS3EDCController::persistState() {
    if (!_state.littleFsReady) return;
    _storage.saveState(_state);
}

void PaperS3EDCController::persistConfig() {
    if (!_state.littleFsReady) return;
    _storage.saveConfig(_state.config);
}

void PaperS3EDCController::scanWifiNetworks() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false, true);
    delay(40);
    int count = WiFi.scanNetworks(false, true);
    _state.wifiNetworkCount = 0;
    if (count <= 0) {
        setStatus("未发现网络", "稍后重试");
        return;
    }
    const int limit = std::min(count, (int)kMaxWifiNetworks);
    for (int index = 0; index < limit; ++index) {
        copyText(_state.wifiNetworks[index].ssid, sizeof(_state.wifiNetworks[index].ssid), WiFi.SSID(index).c_str());
        _state.wifiNetworks[index].rssi = WiFi.RSSI(index);
        _state.wifiNetworks[index].secure = (WiFi.encryptionType(index) != WIFI_AUTH_OPEN);
    }
    _state.wifiNetworkCount = limit;
    _state.focusedWifiIndex = 0;
    setStatus("扫描完成", "选择 Wi‑Fi");
    WiFi.scanDelete();
}

void PaperS3EDCController::appendPasswordChar(const char* label) {
    if (!label || !label[0]) return;
    const size_t len = strlen(_state.passwordBuffer);
    if (len >= sizeof(_state.passwordBuffer) - 1) return;
    if (strcmp(label, "空") == 0) {
        _state.passwordBuffer[len] = ' ';
        _state.passwordBuffer[len + 1] = '\0';
        return;
    }
    _state.passwordBuffer[len] = label[0];
    _state.passwordBuffer[len + 1] = '\0';
}

void PaperS3EDCController::removeLastPasswordChar() {
    const size_t len = strlen(_state.passwordBuffer);
    if (len == 0) return;
    _state.passwordBuffer[len - 1] = '\0';
}

void PaperS3EDCController::connectConfiguredWifi() {
    if (!_state.config.ssid[0]) {
        setStatus("未选网络", "先选 Wi‑Fi");
        return;
    }
    if (!_state.passwordBuffer[0] && _state.config.password[0]) {
        copyText(_state.passwordBuffer, sizeof(_state.passwordBuffer), _state.config.password);
    }
    if (!_state.passwordBuffer[0]) {
        setStatus("密码为空", "继续输入");
        return;
    }
    copyText(_state.config.password, sizeof(_state.config.password), _state.passwordBuffer);
    _state.config.wifiConfigured = true;
    WiFi.mode(WIFI_STA);
    WiFi.begin(_state.config.ssid, _state.config.password);
    setStatus("正在连接 Wi‑Fi", _state.config.ssid);
    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 3000) {
        delay(100);
    }
    _state.config.wifiConnected = WiFi.status() == WL_CONNECTED;
    if (_state.config.wifiConnected) {
        copyText(_state.config.lastSyncLabel, sizeof(_state.config.lastSyncLabel), "网络已连接");
        _state.syncState = SyncState::Idle;
        setStatus("Wi‑Fi 已连接", _state.config.ssid);
    } else {
        copyText(_state.config.lastSyncLabel, sizeof(_state.config.lastSyncLabel), "连接失败");
        _state.syncState = SyncState::Offline;
        setStatus("连接失败", "检查密码");
    }
    persistConfig();
    showPage(AppPage::Settings);
}

void PaperS3EDCController::runManualSync() {
    const bool pushed = _sync.pushPendingEvents(_state);
    const bool pulled = _sync.pullTodayTasks(_state);
    const bool pulledMessages = _sync.pullMessages(_state, kRelayDeviceId);
    refreshDateAndPower();
    refreshCounts();

    if (pushed || pulled || pulledMessages) {
        persistState();
        persistConfig();
        _lastMessagePollMs = millis();
    }

    if (pushed && pulled && pulledMessages) {
        setStatus("同步完成", "任务和消息已更新");
    } else if (pulledMessages && !pushed && !pulled) {
        setStatus("消息已拉取", "relay 已刷新");
    } else if (pushed && pulled) {
        setStatus("同步完成", "已更新任务");
    } else if (pushed) {
        setStatus("仅已上报", "拉取未完成");
    } else if (pulled) {
        setStatus("仅已拉取", "待上报记录");
    }
}

void PaperS3EDCController::maybePollMessages(uint32_t now) {
    if (_state.currentPage == AppPage::WifiPassword) {
        return;
    }

    const uint32_t intervalSec = _state.summary.charging ? _state.pollIntervalChargingSec : _state.pollIntervalBatterySec;
    const uint32_t intervalMs = std::max<uint32_t>(1, intervalSec) * 1000;
    if (_lastMessagePollMs != 0 && (now - _lastMessagePollMs) < intervalMs) {
        return;
    }

    if (_sync.pullMessages(_state, kRelayDeviceId)) {
        persistState();
        persistConfig();
        if (_state.currentPage == AppPage::Messages || _state.currentPage == AppPage::MessageDetail) {
            _renderer.render(_state);
        }
    }
    _lastMessagePollMs = now;
}

void PaperS3EDCController::handleWifiPasswordTouch(int x, int y) {
    if (y < _layout.keyboardTop) return;
    const int row = (y - _layout.keyboardTop) / (_layout.keyboardKeyH + _layout.keyboardGap);
    const int col = (x - _layout.padding) / (_layout.keyboardKeyW + _layout.keyboardGap);
    if (row < 0 || row >= 4 || col < 0 || col >= 10) return;
    const RectBox keyRect = _layout.keyboardKeyRect(row, col);
    if (!keyRect.contains(x, y)) return;

    static const char* lowerRows[4][10] = {
        {"1","2","3","4","5","6","7","8","9","0"},
        {"q","w","e","r","t","y","u","i","o","p"},
        {"a","s","d","f","g","h","j","k","l","<-"},
        {"大","z","x","c","v","b","n","m","空","确定"},
    };
    static const char* upperRows[4][10] = {
        {"1","2","3","4","5","6","7","8","9","0"},
        {"Q","W","E","R","T","Y","U","I","O","P"},
        {"A","S","D","F","G","H","J","K","L","<-"},
        {"小","Z","X","C","V","B","N","M","空","确定"},
    };
    const char* label = _state.passwordUppercase ? upperRows[row][col] : lowerRows[row][col];
    if (strcmp(label, "<-" ) == 0) {
        removeLastPasswordChar();
    } else if (strcmp(label, "大") == 0 || strcmp(label, "小") == 0) {
        _state.passwordUppercase = !_state.passwordUppercase;
    } else if (strcmp(label, "确定") == 0) {
        connectConfiguredWifi();
        return;
    } else {
        appendPasswordChar(label);
    }
    showPage(AppPage::WifiPassword);
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
    if (y < _layout.footerTop + 10) {
        return;
    }
    for (int index = 0; index < 4; ++index) {
        RectBox rect = _layout.footerButtonRect(index);
        if (rect.contains(x, y)) {
            switch (index) {
                case 0:
                    showPage(AppPage::Today);
                    return;
                case 1:
                    showPage(AppPage::Progress);
                    return;
                case 2:
                    showPage(AppPage::Messages);
                    return;
                case 3:
                    showPage(AppPage::Settings);
                    return;
            }
        }
    }
}

void PaperS3EDCController::handleTouch(int x, int y) {
    if (y >= _layout.footerTop) {
        handleFooterTouch(x, y);
        return;
    }

    if (_state.currentPage == AppPage::Today) {
        for (size_t index = 0; index < _state.taskCount; ++index) {
            RectBox row = _layout.taskRowRect((int)index);
            if (!row.contains(x, y)) {
                continue;
            }
            _state.selectedTaskIndex = (int)index;
            if (_state.tasks[index].status != TaskStatus::Done) {
                _state.armedTaskIndex = (int)index;
                _state.tasks[index].status = TaskStatus::Armed;
                setStatus("任务已选中", "确认完成");
                showPage(AppPage::Confirm);
            }
            return;
        }
        return;
    }

    if (_state.currentPage == AppPage::Confirm) {
        RectBox backRect = _layout.confirmBackRect();
        RectBox doneRect = _layout.confirmDoneRect();
        if (backRect.contains(x, y)) {
            if (_state.armedTaskIndex >= 0
                && _state.armedTaskIndex < (int)_state.taskCount
                && _state.tasks[_state.armedTaskIndex].status == TaskStatus::Armed) {
                _state.tasks[_state.armedTaskIndex].status = TaskStatus::Pending;
            }
            _state.armedTaskIndex = -1;
            setStatus("已取消", "返回今日页");
            refreshCounts();
            showPage(AppPage::Today);
            return;
        }
        if (doneRect.contains(x, y)
            && _state.armedTaskIndex >= 0
            && _state.armedTaskIndex < (int)_state.taskCount) {
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
            setStatus("任务完成", "积分已加");
            persistState();
            showPage(AppPage::Today);
            return;
        }
        return;
    }

    if (_state.currentPage == AppPage::Messages) {
        const int listTop = _layout.bodyTop + 62;
        for (size_t index = 0; index < _state.messageCount && index < 4; ++index) {
            RectBox row{_layout.padding,
                        listTop + (int)index * (kMessageRowHeight + kMessageRowGap),
                        _layout.width - _layout.padding * 2,
                        kMessageRowHeight};
            if (!row.contains(x, y)) {
                continue;
            }
            _state.messagePage.selectedMessageIndex = (int)index;
            _state.messagePage.selectedMessageDetailIndex = (int)index;
            _state.messagePage.messageScrollOffset = 0;
            _state.messages[index].read = true;
            setStatus("打开消息", _state.messages[index].title);
            showPage(AppPage::MessageDetail);
            return;
        }
        return;
    }

    if (_state.currentPage == AppPage::MessageDetail) {
        const RectBox backRow{_layout.padding,
                              _layout.bodyBottom - kMessageDetailFooterHeight,
                              _layout.width - _layout.padding * 2,
                              kMessageDetailFooterHeight};
        if (backRow.contains(x, y)) {
            setStatus("返回消息页", "继续查看列表");
            showPage(AppPage::Messages);
            return;
        }

        if (y < kMessageDetailBodyTop || y > (_layout.bodyBottom - kMessageDetailFooterHeight - 8)) {
            return;
        }

        if (y < (_layout.bodyTop + (_layout.bodyBottom - _layout.bodyTop) / 2)) {
            _state.messagePage.messageScrollOffset = std::max(0, _state.messagePage.messageScrollOffset - kMessageScrollStep);
            setStatus("消息详情", "向上滚动");
        } else {
            _state.messagePage.messageScrollOffset += kMessageScrollStep;
            setStatus("消息详情", "向下滚动");
        }
        showPage(AppPage::MessageDetail);
        return;
    }

    if (_state.currentPage == AppPage::Settings) {
        for (int index = 0; index < 5; ++index) {
            RectBox row = _layout.settingsRowRect(index);
            if (!row.contains(x, y)) {
                continue;
            }
            _state.focusedSettingsIndex = index;
            if (index == 0 || index == 1) {
                uint32_t& value = (index == 0) ? _state.pollIntervalChargingSec : _state.pollIntervalBatterySec;
                if (x < row.x + row.w / 2) {
                    value = (value <= kMinPollIntervalSec) ? kMinPollIntervalSec : (value - kPollIntervalStepSec);
                } else {
                    value = std::min<uint32_t>(kMaxPollIntervalSec, value + kPollIntervalStepSec);
                }
                setStatus(index == 0 ? "插电拉取间隔" : "电池拉取间隔", x < row.x + row.w / 2 ? "已调低" : "已调高");
                persistState();
                showPage(AppPage::Settings);
            } else if (index == 2) {
                scanWifiNetworks();
                showPage(AppPage::WifiList);
            } else if (index == 3) {
                runManualSync();
                showPage(AppPage::SyncStatus);
            } else {
                showPage(AppPage::Today);
            }
            return;
        }
        return;
    }

    if (_state.currentPage == AppPage::WifiList) {
        for (size_t index = 0; index < std::min<size_t>(_state.wifiNetworkCount, 6); ++index) {
            RectBox row = _layout.wifiRowRect((int)index);
            if (!row.contains(x, y)) {
                continue;
            }
            _state.focusedWifiIndex = (int)index;
            copyText(_state.config.ssid, sizeof(_state.config.ssid), _state.wifiNetworks[index].ssid);
            memset(_state.passwordBuffer, 0, sizeof(_state.passwordBuffer));
            _state.passwordUppercase = false;
            setStatus("已选择网络", _state.config.ssid);
            showPage(AppPage::WifiPassword);
            return;
        }
        return;
    }

    if (_state.currentPage == AppPage::WifiPassword) {
        handleWifiPasswordTouch(x, y);
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
    _layout = LayoutMetrics::portrait(M5.Display.width(), M5.Display.height());
    mountStorage();
    _state.fontReady = _fonts.load(_state.littleFsReady, _state.sdReady);
    seedFakeState();
    if (_state.littleFsReady) {
        _storage.loadConfig(_state.config);
        if (_storage.loadState(_state)) {
            refreshDateAndPower();
            refreshCounts();
            setStatus("已恢复状态", _state.config.wifiConfigured ? _state.config.ssid : "本地缓存");
        }
    }
    if (_state.config.wifiConfigured) {
        connectConfiguredWifi();
    }
    _lastMessagePollMs = millis();

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
    maybePollMessages(now);

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
