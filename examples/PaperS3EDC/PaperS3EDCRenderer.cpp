#include "PaperS3EDCRenderer.h"

#include <algorithm>
#include <stdio.h>

namespace papers3edc {
namespace {
constexpr int kStatusHeight = 54;
constexpr int kTaskMarkerSize = 22;
constexpr int kSectionGap = 16;
constexpr int kTitleLineHeight = 38;
constexpr int kKeyboardRows = 4;
constexpr int kKeyboardCols = 10;
constexpr const char* kTabLabels[] = {"今日", "成长", "屏保", "配置"};
constexpr const char* kSettingsLabels[] = {"Wi‑Fi 配置", "手动同步", "返回今日"};
constexpr const char* kWeekdayCn[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};

const char* pageTitle(AppPage page) {
    switch (page) {
        case AppPage::Today: return "今日安排";
        case AppPage::Confirm: return "确认完成";
        case AppPage::Progress: return "成长进度";
        case AppPage::Screensaver: return "今日纸面";
        case AppPage::Settings: return "设备配置";
        case AppPage::WifiList: return "选择网络";
        case AppPage::WifiPassword: return "输入密码";
        case AppPage::SyncStatus: return "同步结果";
        default: return "PaperS3 EDC";
    }
}

const char* syncStateName(SyncState state) {
    switch (state) {
        case SyncState::Offline: return "离线";
        case SyncState::Pending: return "同步中";
        case SyncState::Success: return "已同步";
        case SyncState::Failed: return "失败";
        case SyncState::Idle:
        default:
            return "待机";
    }
}

const char* passwordKeyLabel(int row, int col, bool uppercase) {
    static const char* lowerRows[kKeyboardRows][kKeyboardCols] = {
        {"1","2","3","4","5","6","7","8","9","0"},
        {"q","w","e","r","t","y","u","i","o","p"},
        {"a","s","d","f","g","h","j","k","l","<-"},
        {"大","z","x","c","v","b","n","m","空","确定"},
    };
    static const char* upperRows[kKeyboardRows][kKeyboardCols] = {
        {"1","2","3","4","5","6","7","8","9","0"},
        {"Q","W","E","R","T","Y","U","I","O","P"},
        {"A","S","D","F","G","H","J","K","L","<-"},
        {"小","Z","X","C","V","B","N","M","空","确定"},
    };
    return uppercase ? upperRows[row][col] : lowerRows[row][col];
}
}  // namespace

PaperS3EDCRenderer::PaperS3EDCRenderer(lgfx::LGFXBase* display, PaperS3EDCFontManager* fonts)
    : _display(display), _fonts(fonts) {}

void PaperS3EDCRenderer::begin() {
    _display->setRotation(0);
    _display->setTextColor(TFT_BLACK, TFT_WHITE);
    _display->setTextSize(1);
    _display->setFont(&fonts::Font2);
    _width = _display->width();
    _height = _display->height();
    _layout = LayoutMetrics::portrait(_width, _height);
}

void PaperS3EDCRenderer::applyEpdMode() {
    if (M5.Display.isEPD()) {
        M5.Display.setEpdMode(epd_mode_t::epd_text);
    }
}

void PaperS3EDCRenderer::displayFrame() {
    applyEpdMode();
    _display->display();
}

void PaperS3EDCRenderer::render(const AppState& state) {
    _display->fillScreen(TFT_WHITE);
    drawStatusBar(state);
    switch (state.currentPage) {
        case AppPage::Today: drawTodayPage(state); break;
        case AppPage::Confirm: drawConfirmPage(state); break;
        case AppPage::Progress: drawProgressPage(state); break;
        case AppPage::Screensaver: drawScreensaverPage(state); break;
        case AppPage::Settings: drawSettingsPage(state); break;
        case AppPage::WifiList: drawWifiListPage(state); break;
        case AppPage::WifiPassword: drawWifiPasswordPage(state); break;
        case AppPage::SyncStatus: drawSyncStatusPage(state); break;
    }
    drawBottomTabs(state);
    displayFrame();
}

void PaperS3EDCRenderer::drawStatusBar(const AppState& state) {
    _display->fillRect(0, 0, _width, _layout.statusHeight, TFT_WHITE);
    _display->drawLine(0, _layout.statusHeight - 1, _width, _layout.statusHeight - 1, TFT_BLACK);
    _display->setFont(&fonts::Font2);
    _display->setCursor(_layout.padding, 10);
    _display->printf("%s", pageTitle(state.currentPage));

    drawBatteryBar(_width - 132, 10, 42, 18, state.summary.batteryPercent, state.summary.charging);

    _display->setCursor(_width - 82, 12);
    _display->printf("%u/%u", state.summary.completedTasks, state.summary.totalTasks);
    _display->setCursor(_width - 82, 30);
    _display->printf("%s", syncStateName(state.syncState));
}

void PaperS3EDCRenderer::drawBottomTabs(const AppState& state) {
    const int y = _layout.footerTop;
    _display->drawLine(0, y, _width, y, TFT_BLACK);
    const int buttonWidth = _layout.footerButtonWidth();
    for (int index = 0; index < 4; ++index) {
        const int x = _layout.footerButtonRect(index).x;
        const bool active = ((index == 0 && state.currentPage == AppPage::Today)
                          || (index == 1 && state.currentPage == AppPage::Progress)
                          || (index == 2 && state.currentPage == AppPage::Screensaver)
                          || (index == 3 && (state.currentPage == AppPage::Settings || state.currentPage == AppPage::WifiList || state.currentPage == AppPage::WifiPassword || state.currentPage == AppPage::SyncStatus)));
        _display->drawRect(x, y + 10, buttonWidth, _layout.footerButtonHeight, TFT_BLACK);
        if (active) {
            _display->fillRect(x + 1, y + 11, buttonWidth - 2, _layout.footerButtonHeight - 2, TFT_BLACK);
            _display->setTextColor(TFT_WHITE, TFT_BLACK);
        } else {
            _display->setTextColor(TFT_BLACK, TFT_WHITE);
        }
        _display->setCursor(x + buttonWidth / 2 - 14, y + 18);
        _display->print(kTabLabels[index]);
    }
    _display->setTextColor(TFT_BLACK, TFT_WHITE);
    _display->setCursor(_layout.padding, y + 50);
    _display->printf("%s", state.statusLine);
    _display->setCursor(_width / 2, y + 48);
    _display->printf("%s", state.statusDetail);
}

void PaperS3EDCRenderer::drawTaskMarker(int x, int y, int w, int h, TaskStatus status, bool selected) {
    _display->drawRect(x, y, w, h, TFT_BLACK);
    if (selected) {
        _display->fillRect(x + 2, y + 2, w - 4, h - 4, TFT_BLACK);
        _display->drawFastHLine(x + 4, y + h / 2, w - 8, TFT_WHITE);
        return;
    }
    if (status == TaskStatus::Done) {
        _display->fillRect(x + 2, y + 2, w - 4, h - 4, TFT_BLACK);
        _display->drawFastHLine(x + 4, y + h / 2, w - 8, TFT_WHITE);
    } else if (status == TaskStatus::Armed) {
        _display->drawRect(x + 3, y + 3, w - 6, h - 6, TFT_BLACK);
    }
}

void PaperS3EDCRenderer::drawTodayPage(const AppState& state) {
    _display->setFont(&fonts::Font2);
    _display->setCursor(_layout.padding, _layout.bodyTop);
    _display->printf("%s · %s", state.summary.dateTitle, state.summary.weekLabel);

    if (_fonts && _fonts->isReady()) {
        _fonts->drawText("今天要完成的事", _layout.padding, _layout.bodyTop + 28, _width - _layout.padding * 2, 36, false);
    }

    const int listTop = _layout.taskListTop;
    const int listBottom = _layout.bodyBottom;
    const int availableRows = std::max(1, (listBottom - listTop) / (_layout.taskRowHeight + _layout.taskGap));
    const bool fallback = (int)state.taskCount > availableRows;

    if (fallback) {
        _display->drawRect(_layout.padding, listTop, _width - _layout.padding * 2, 164, TFT_BLACK);
        _display->setCursor(_layout.padding + 16, listTop + 20);
        _display->print("今日任务较多，已切换为总览模式");
        _display->setCursor(_layout.padding + 16, listTop + 58);
        _display->printf("共 %u 项", state.summary.totalTasks);
        _display->setCursor(_layout.padding + 16, listTop + 90);
        _display->printf("已完成 %u 项", state.summary.completedTasks);
        _display->setCursor(_layout.padding + 16, listTop + 122);
        _display->printf("今日积分 %u", state.summary.dayPoints);
        return;
    }

    for (size_t index = 0; index < state.taskCount; ++index) {
        const int rowY = listTop + (int)index * (_layout.taskRowHeight + _layout.taskGap);
        const bool selected = ((int)index == state.selectedTaskIndex);
        _display->drawRect(_layout.padding, rowY, _width - _layout.padding * 2, _layout.taskRowHeight, TFT_BLACK);
        if (selected) {
            _display->fillRect(_layout.padding + 1, rowY + 1, _width - _layout.padding * 2 - 2, _layout.taskRowHeight - 2, TFT_WHITE);
        }
        drawTaskMarker(_layout.padding + 12, rowY + 18, kTaskMarkerSize, kTaskMarkerSize, state.tasks[index].status, selected && state.currentPage == AppPage::Confirm);
        if (_fonts && _fonts->isReady()) {
            _fonts->drawText(state.tasks[index].title, _layout.padding + 48, rowY + 10, _width - _layout.padding * 2 - 120, 40, true);
        } else {
            _display->setCursor(_layout.padding + 48, rowY + 16);
            _display->printf("%s", state.tasks[index].title);
        }
        _display->setCursor(_width - 72, rowY + 18);
        _display->printf("+%u", state.tasks[index].points);
    }
}

void PaperS3EDCRenderer::drawConfirmPage(const AppState& state) {
    if (state.armedTaskIndex < 0 || state.armedTaskIndex >= (int)state.taskCount) {
        drawFallbackBodyMessage("没有选中任务", "请返回今日页重新选择");
        return;
    }
    const TaskItem& task = state.tasks[state.armedTaskIndex];
    _display->drawRect(_layout.padding, _layout.bodyTop, _width - _layout.padding * 2, _layout.confirmCardHeight, TFT_BLACK);
    _display->setCursor(_layout.padding + 18, _layout.bodyTop + 20);
    _display->print("确认完成以下任务");
    if (_fonts && _fonts->isReady()) {
        _fonts->drawText(task.title, _layout.padding + 18, _layout.bodyTop + 58, _width - _layout.padding * 2 - 36, 110, true);
    }
    _display->setCursor(_layout.padding + 18, _layout.bodyTop + 184);
    _display->printf("完成后将获得 %u 积分", task.points);

    const RectBox backRect = _layout.confirmBackRect();
    const RectBox doneRect = _layout.confirmDoneRect();
    _display->drawRect(backRect.x, backRect.y, backRect.w, backRect.h, TFT_BLACK);
    _display->setCursor(backRect.x + backRect.w / 2 - 14, backRect.y + 12);
    _display->print("返回");
    _display->fillRect(doneRect.x, doneRect.y, doneRect.w, doneRect.h, TFT_BLACK);
    _display->setTextColor(TFT_WHITE, TFT_BLACK);
    _display->setCursor(doneRect.x + doneRect.w / 2 - 28, doneRect.y + 12);
    _display->print("确认完成");
    _display->setTextColor(TFT_BLACK, TFT_WHITE);
}

void PaperS3EDCRenderer::drawProgressPage(const AppState& state) {
    _display->setCursor(_layout.padding, _layout.bodyTop);
    _display->printf("累计积分 %u", state.summary.totalPoints);
    _display->setCursor(_layout.padding, _layout.bodyTop + 30);
    _display->printf("连续 %u 天", state.summary.streakDays);
    _display->drawRect(_layout.padding, _layout.bodyTop + 74, _width - _layout.padding * 2, 120, TFT_BLACK);
    _display->setCursor(_layout.padding + 14, _layout.bodyTop + 92);
    _display->printf("今日完成 %u / %u", state.summary.completedTasks, state.summary.totalTasks);
    _display->setCursor(_layout.padding + 14, _layout.bodyTop + 128);
    _display->printf("今日新增积分 %u", state.summary.dayPoints);

    const int gridTop = _layout.bodyTop + 230;
    const int cellW = 24;
    const int cellH = 20;
    const int gap = 6;
    for (size_t index = 0; index < kContributionCellCount; ++index) {
        const int row = index / 7;
        const int col = index % 7;
        const int x = _layout.padding + col * (cellW + gap);
        const int y = gridTop + row * (cellH + gap);
        _display->drawRect(x, y, cellW, cellH, TFT_BLACK);
        const uint8_t level = state.contribution[index].level;
        if (level == 1) {
            _display->fillRect(x + 1, y + 1, cellW - 2, cellH - 2, TFT_LIGHTGRAY);
        } else if (level == 2) {
            _display->fillRect(x + 1, y + 1, cellW - 2, cellH - 2, 0xBDF7);
        } else if (level >= 3) {
            _display->fillRect(x + 1, y + 1, cellW - 2, cellH - 2, TFT_BLACK);
        }
    }

    _display->setCursor(_layout.padding, gridTop + 124);
    _display->print("成长贡献图");
    _display->setCursor(_layout.padding, gridTop + 152);
    _display->print("浅灰=开始");
    _display->setCursor(_layout.padding, gridTop + 180);
    _display->print("深灰=全部完成");
}

void PaperS3EDCRenderer::drawScreensaverPage(const AppState& state) {
    if (_fonts && _fonts->isReady()) {
        _fonts->drawText(state.summary.dateTitle, _layout.padding, _layout.bodyTop, _width - _layout.padding * 2, 56, false);
    } else {
        _display->setCursor(_layout.padding, _layout.bodyTop);
        _display->printf("%s", state.summary.dateTitle);
    }
    _display->setCursor(_layout.padding, _layout.bodyTop + 62);
    _display->printf("%s", state.summary.weekLabel);
    _display->drawLine(_layout.padding, _layout.bodyTop + 92, _width - _layout.padding, _layout.bodyTop + 92, TFT_BLACK);
    _display->setCursor(_layout.padding, _layout.bodyTop + 116);
    _display->print("今日安排");

    const bool fallback = state.taskCount > 4;
    if (fallback) {
        _display->setCursor(_layout.padding, _layout.bodyTop + 168);
        _display->printf("今日共 %u 项任务", state.summary.totalTasks);
        _display->setCursor(_layout.padding, _layout.bodyTop + 206);
        _display->printf("已完成 %u 项", state.summary.completedTasks);
        _display->setCursor(_layout.padding, _layout.bodyTop + 244);
        _display->printf("累计积分 %u", state.summary.totalPoints);
    } else {
        for (size_t index = 0; index < state.taskCount; ++index) {
            const int y = _layout.bodyTop + 154 + (int)index * 48;
            _display->drawRect(_layout.padding, y, 16, 16, TFT_BLACK);
            if (state.tasks[index].status == TaskStatus::Done) {
                _display->fillRect(_layout.padding + 2, y + 2, 12, 12, TFT_BLACK);
            }
            _display->setCursor(_layout.padding + 28, y - 2);
            _display->printf("%s", state.tasks[index].title);
        }
    }

    drawNotionLineArt(_width - 210, _layout.bodyTop + 336, 158, 148);
}

void PaperS3EDCRenderer::drawSettingsPage(const AppState& state) {
    _display->setCursor(_layout.padding, _layout.bodyTop);
    _display->print("设备配置");
    _display->setCursor(_layout.padding, _layout.bodyTop + 28);
    _display->printf("网络：%s", state.config.wifiConfigured ? state.config.ssid : "未配置");
    _display->setCursor(_layout.padding, _layout.bodyTop + 54);
    _display->printf("同步：%s", state.config.lastSyncLabel);

    for (int index = 0; index < 3; ++index) {
        const int y = _layout.settingsRowRect(index).y;
        const bool active = (index == state.focusedSettingsIndex);
        _display->drawRect(_layout.padding, y, _width - _layout.padding * 2, 42, TFT_BLACK);
        if (active) {
            _display->fillRect(_layout.padding + 1, y + 1, _width - _layout.padding * 2 - 2, 40, TFT_BLACK);
            _display->setTextColor(TFT_WHITE, TFT_BLACK);
        }
        _display->setCursor(_layout.padding + 14, y + 12);
        _display->printf("%s", kSettingsLabels[index]);
        _display->setTextColor(TFT_BLACK, TFT_WHITE);
    }
}

void PaperS3EDCRenderer::drawWifiListPage(const AppState& state) {
    _display->setCursor(_layout.padding, _layout.bodyTop);
    _display->print("选择一个网络");
    const size_t count = std::max<size_t>(state.wifiNetworkCount, 1);
    for (size_t index = 0; index < count && index < 6; ++index) {
        const int y = _layout.wifiRowRect((int)index).y;
        const bool active = ((int)index == state.focusedWifiIndex);
        _display->drawRect(_layout.padding, y, _width - _layout.padding * 2, 40, TFT_BLACK);
        if (active) {
            _display->fillRect(_layout.padding + 1, y + 1, _width - _layout.padding * 2 - 2, 38, TFT_BLACK);
            _display->setTextColor(TFT_WHITE, TFT_BLACK);
        }
        _display->setCursor(_layout.padding + 12, y + 11);
        if (state.wifiNetworkCount == 0) {
            _display->print("重新扫描中…");
        } else {
            _display->printf("%s", state.wifiNetworks[index].ssid);
            _display->setCursor(_width - 92, y + 11);
            _display->printf("%ddBm", state.wifiNetworks[index].rssi);
        }
        _display->setTextColor(TFT_BLACK, TFT_WHITE);
    }
}

void PaperS3EDCRenderer::drawWifiPasswordPage(const AppState& state) {
    _display->setCursor(_layout.padding, _layout.bodyTop);
    _display->printf("网络：%s", state.config.ssid[0] ? state.config.ssid : "未选择");
    _display->drawRect(_layout.padding, _layout.bodyTop + 42, _width - _layout.padding * 2, 44, TFT_BLACK);
    _display->setCursor(_layout.padding + 12, _layout.bodyTop + 56);
    if (state.passwordBuffer[0]) {
        _display->printf("%s", state.passwordBuffer);
    } else {
        _display->print("请输入密码");
    }
    drawKeyboardGrid(state, _layout.padding, _layout.keyboardTop, _layout.keyboardKeyW, _layout.keyboardKeyH, _layout.keyboardGap);
}

void PaperS3EDCRenderer::drawSyncStatusPage(const AppState& state) {
    drawFallbackBodyMessage(state.statusLine, state.statusDetail);
}

void PaperS3EDCRenderer::drawBatteryBar(int x, int y, int width, int height, int32_t percent, bool charging) {
    _display->drawRect(x, y, width, height, TFT_BLACK);
    _display->drawRect(x + width, y + 5, 4, height - 10, TFT_BLACK);
    if (percent < 0) {
        return;
    }
    const int innerWidth = std::max(0, std::min(width - 4, (int)((width - 4) * percent / 100)));
    _display->fillRect(x + 2, y + 2, innerWidth, height - 4, TFT_BLACK);
    if (charging) {
        _display->drawFastVLine(x + width / 2, y + 4, height - 8, TFT_WHITE);
    }
}

void PaperS3EDCRenderer::drawNotionLineArt(int x, int y, int w, int h) {
    _display->drawRoundRect(x + 10, y + 10, w - 20, h - 20, 8, TFT_BLACK);
    _display->drawCircle(x + 42, y + 44, 16, TFT_BLACK);
    _display->drawLine(x + 42, y + 62, x + 42, y + 106, TFT_BLACK);
    _display->drawLine(x + 24, y + 84, x + 60, y + 84, TFT_BLACK);
    _display->drawRect(x + 96, y + 28, 58, 32, TFT_BLACK);
    _display->drawLine(x + 96, y + 72, x + 154, y + 114, TFT_BLACK);
    _display->drawLine(x + 154, y + 72, x + 96, y + 114, TFT_BLACK);
    _display->drawRect(x + 82, y + 126, 76, 18, TFT_BLACK);
}

void PaperS3EDCRenderer::drawKeyboardGrid(const AppState& state, int x, int y, int keyW, int keyH, int gap) {
    (void)state;
    for (int row = 0; row < kKeyboardRows; ++row) {
        for (int col = 0; col < kKeyboardCols; ++col) {
            const int keyX = x + col * (keyW + gap);
            const int keyY = y + row * (keyH + gap);
            if (keyX + keyW > _width - _layout.padding) {
                continue;
            }
            _display->drawRect(keyX, keyY, keyW, keyH, TFT_BLACK);
            _display->setCursor(keyX + 14, keyY + 14);
            _display->printf("%s", passwordKeyLabel(row, col, state.passwordUppercase));
        }
    }
}

void PaperS3EDCRenderer::drawFallbackBodyMessage(const char* title, const char* detail) {
    _display->drawRect(_layout.padding, _layout.bodyTop, _width - _layout.padding * 2, 180, TFT_BLACK);
    _display->setCursor(_layout.padding + 18, _layout.bodyTop + 26);
    _display->printf("%s", title);
    _display->setCursor(_layout.padding + 18, _layout.bodyTop + 72);
    _display->printf("%s", detail);
}

}  // namespace papers3edc
