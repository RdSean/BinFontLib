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
constexpr int kMessageCardHeight = 82;
constexpr int kMessageCardGap = 12;
constexpr int kMessageDetailBodyTop = 140;
constexpr int kMessageDetailFooterHeight = 44;
constexpr int kMessageScrollStep = 84;
constexpr const char* kTabLabels[] = {"今日", "成长", "消息", "配置"};
constexpr int kSettingsRowCount = 5;
constexpr const char* kWeekdayCn[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};

const char* pageTitle(AppPage page) {
    switch (page) {
        case AppPage::Today: return "今日安排";
        case AppPage::Confirm: return "确认完成";
        case AppPage::Progress: return "成长进度";
        case AppPage::Messages: return "消息收件箱";
        case AppPage::MessageDetail: return "消息详情";
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

void PaperS3EDCRenderer::drawUiText(const char* text, int x, int y, int width, int height, bool wrap) {
    if (_fonts && _fonts->isReady()) {
        _fonts->drawText(text, x, y, width, height, wrap);
        return;
    }

    _display->setFont(&fonts::Font2);
    _display->setCursor(x, y + 14);
    _display->print(text);
}

void PaperS3EDCRenderer::drawUiTextCentered(const char* text, int x, int y, int width, int height) {
    if (_fonts && _fonts->isReady()) {
        _fonts->drawText(text, x + 6, y + 6, width - 12, height - 12, false);
        return;
    }

    _display->setFont(&fonts::Font2);
    _display->setCursor(x + (width / 2) - 14, y + 12);
    _display->print(text);
}

void PaperS3EDCRenderer::render(const AppState& state) {
    _display->fillScreen(TFT_WHITE);
    drawStatusBar(state);
    switch (state.currentPage) {
        case AppPage::Today: drawTodayPage(state); break;
        case AppPage::Confirm: drawConfirmPage(state); break;
        case AppPage::Progress: drawProgressPage(state); break;
        case AppPage::Messages: drawMessagesPage(state); break;
        case AppPage::MessageDetail: drawMessageDetailPage(state); break;
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
    drawUiText(pageTitle(state.currentPage), _layout.padding, 8, 188, 28, false);

    drawBatteryBar(_width - 132, 10, 42, 18, state.summary.batteryPercent, state.summary.charging);

    _display->setFont(&fonts::Font2);
    _display->setCursor(_width - 82, 12);
    _display->printf("%u/%u", state.summary.completedTasks, state.summary.totalTasks);
    drawUiText(syncStateName(state.syncState), _width - 82, 28, 72, 20, false);
}

void PaperS3EDCRenderer::drawBottomTabs(const AppState& state) {
    const int y = _layout.footerTop;
    _display->drawLine(0, y, _width, y, TFT_BLACK);
    const int buttonWidth = _layout.footerButtonWidth();
    for (int index = 0; index < 4; ++index) {
        const int x = _layout.footerButtonRect(index).x;
        const bool active = ((index == 0 && state.currentPage == AppPage::Today)
                          || (index == 1 && state.currentPage == AppPage::Progress)
                          || (index == 2 && (state.currentPage == AppPage::Messages || state.currentPage == AppPage::MessageDetail))
                          || (index == 3 && (state.currentPage == AppPage::Settings || state.currentPage == AppPage::WifiList || state.currentPage == AppPage::WifiPassword || state.currentPage == AppPage::SyncStatus)));
        _display->drawRect(x, y + 10, buttonWidth, _layout.footerButtonHeight, TFT_BLACK);
        if (active) {
            _display->fillRect(x + 1, y + 11, buttonWidth - 2, _layout.footerButtonHeight - 2, TFT_BLACK);
            _display->setTextColor(TFT_WHITE, TFT_BLACK);
        } else {
            _display->setTextColor(TFT_BLACK, TFT_WHITE);
        }
        drawUiTextCentered(kTabLabels[index], x, y + 10, buttonWidth, _layout.footerButtonHeight);
    }
    _display->setTextColor(TFT_BLACK, TFT_WHITE);
    drawUiText(state.statusLine, _layout.padding, y + 46, (_width / 2) - _layout.padding - 8, 20, false);
    drawUiText(state.statusDetail, _width / 2, y + 46, (_width / 2) - _layout.padding, 20, false);
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
    char dateLine[64];
    snprintf(dateLine, sizeof(dateLine), "%s %s", state.summary.dateTitle, state.summary.weekLabel);
    drawUiText(dateLine, _layout.padding, _layout.bodyTop, _width - _layout.padding * 2, 24, false);

    if (_fonts && _fonts->isReady()) {
        _fonts->drawText("今天要完成的事", _layout.padding, _layout.bodyTop + 28, _width - _layout.padding * 2, 36, false);
    }

    const int listTop = _layout.taskListTop;
    const int listBottom = _layout.bodyBottom;
    const int availableRows = std::max(1, (listBottom - listTop) / (_layout.taskRowHeight + _layout.taskGap));
    const bool fallback = (int)state.taskCount > availableRows;

    if (fallback) {
        _display->drawRect(_layout.padding, listTop, _width - _layout.padding * 2, 164, TFT_BLACK);
        char line[48];
        drawUiText("今日任务较多，已切换为总览模式", _layout.padding + 16, listTop + 18, _width - _layout.padding * 2 - 32, 26, false);
        snprintf(line, sizeof(line), "共 %u 项", state.summary.totalTasks);
        drawUiText(line, _layout.padding + 16, listTop + 56, 120, 24, false);
        snprintf(line, sizeof(line), "已完成 %u 项", state.summary.completedTasks);
        drawUiText(line, _layout.padding + 16, listTop + 88, 160, 24, false);
        snprintf(line, sizeof(line), "今日积分 %u", state.summary.dayPoints);
        drawUiText(line, _layout.padding + 16, listTop + 120, 160, 24, false);
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
    drawUiText("确认完成以下任务", _layout.padding + 18, _layout.bodyTop + 18, _width - _layout.padding * 2 - 36, 26, false);
    if (_fonts && _fonts->isReady()) {
        _fonts->drawText(task.title, _layout.padding + 18, _layout.bodyTop + 58, _width - _layout.padding * 2 - 36, 110, true);
    }
    char confirmLine[48];
    snprintf(confirmLine, sizeof(confirmLine), "完成后将获得 %u 积分", task.points);
    drawUiText(confirmLine, _layout.padding + 18, _layout.bodyTop + 182, _width - _layout.padding * 2 - 36, 24, false);

    const RectBox backRect = _layout.confirmBackRect();
    const RectBox doneRect = _layout.confirmDoneRect();
    _display->drawRect(backRect.x, backRect.y, backRect.w, backRect.h, TFT_BLACK);
    drawUiTextCentered("返回", backRect.x, backRect.y, backRect.w, backRect.h);
    _display->fillRect(doneRect.x, doneRect.y, doneRect.w, doneRect.h, TFT_BLACK);
    _display->setTextColor(TFT_WHITE, TFT_BLACK);
    drawUiTextCentered("确认完成", doneRect.x, doneRect.y, doneRect.w, doneRect.h);
    _display->setTextColor(TFT_BLACK, TFT_WHITE);
}

void PaperS3EDCRenderer::drawProgressPage(const AppState& state) {
    char line[48];
    snprintf(line, sizeof(line), "累计积分 %u", state.summary.totalPoints);
    drawUiText(line, _layout.padding, _layout.bodyTop, 200, 24, false);
    snprintf(line, sizeof(line), "连续 %u 天", state.summary.streakDays);
    drawUiText(line, _layout.padding, _layout.bodyTop + 30, 160, 24, false);
    _display->drawRect(_layout.padding, _layout.bodyTop + 74, _width - _layout.padding * 2, 120, TFT_BLACK);
    snprintf(line, sizeof(line), "今日完成 %u / %u", state.summary.completedTasks, state.summary.totalTasks);
    drawUiText(line, _layout.padding + 14, _layout.bodyTop + 90, 220, 24, false);
    snprintf(line, sizeof(line), "今日新增积分 %u", state.summary.dayPoints);
    drawUiText(line, _layout.padding + 14, _layout.bodyTop + 126, 220, 24, false);

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

    drawUiText("成长贡献图", _layout.padding, gridTop + 122, 140, 24, false);
    drawUiText("浅灰=开始", _layout.padding, gridTop + 150, 120, 24, false);
    drawUiText("深灰=全部完成", _layout.padding, gridTop + 178, 160, 24, false);
}

void PaperS3EDCRenderer::drawMessagesPage(const AppState& state) {
    char line[96];
    snprintf(line, sizeof(line), "本地缓存 %u 条", (unsigned)state.messageCount);
    drawUiText(line, _layout.padding, _layout.bodyTop, 180, 24, false);
    drawUiText("点击消息查看详情", _layout.padding, _layout.bodyTop + 28, 180, 24, false);

    if (state.messageCount == 0) {
        drawFallbackBodyMessage("暂无消息", "等待下一次拉取");
        return;
    }

    const int listTop = _layout.bodyTop + 62;
    for (size_t index = 0; index < state.messageCount && index < 4; ++index) {
        const int y = listTop + (int)index * (kMessageCardHeight + kMessageCardGap);
        const bool selected = ((int)index == state.messagePage.selectedMessageIndex);
        _display->drawRect(_layout.padding, y, _width - _layout.padding * 2, kMessageCardHeight, TFT_BLACK);
        if (selected) {
            _display->fillRect(_layout.padding + 1, y + 1, _width - _layout.padding * 2 - 2, kMessageCardHeight - 2, TFT_BLACK);
            _display->setTextColor(TFT_WHITE, TFT_BLACK);
        }

        const MessageItem& item = state.messages[index];
        if (!item.read) {
            drawUiText("未读", _layout.padding + 12, y + 8, 48, 20, false);
        }
        drawUiText(item.title, _layout.padding + 64, y + 8, _width - _layout.padding * 2 - 150, 22, false);
        drawUiText(item.body, _layout.padding + 12, y + 34, _width - _layout.padding * 2 - 24, 20, false);
        drawUiText(item.sent_at, _width - 108, y + 8, 80, 20, false);
        drawUiText(item.source, _layout.padding + 12, y + 56, 120, 18, false);
        _display->setTextColor(TFT_BLACK, TFT_WHITE);
    }
}

void PaperS3EDCRenderer::drawMessageDetailPage(const AppState& state) {
    if (state.messagePage.selectedMessageDetailIndex < 0
        || state.messagePage.selectedMessageDetailIndex >= (int)state.messageCount) {
        drawFallbackBodyMessage("没有选中消息", "返回消息页重新选择");
        return;
    }

    const MessageItem& item = state.messages[state.messagePage.selectedMessageDetailIndex];
    _display->drawRect(_layout.padding, _layout.bodyTop, _width - _layout.padding * 2, 56, TFT_BLACK);
    drawUiText(item.title, _layout.padding + 12, _layout.bodyTop + 8, _width - _layout.padding * 2 - 24, 20, false);
    char metaLine[96];
    snprintf(metaLine, sizeof(metaLine), "%s | %s", item.source, item.sent_at);
    drawUiText(metaLine, _layout.padding + 12, _layout.bodyTop + 30, _width - _layout.padding * 2 - 24, 18, false);

    const int bodyHeight = _layout.bodyBottom - kMessageDetailBodyTop - kMessageDetailFooterHeight - 8;
    _display->drawRect(_layout.padding, kMessageDetailBodyTop, _width - _layout.padding * 2, bodyHeight, TFT_BLACK);
    drawUiText(item.body,
               _layout.padding + 12,
               kMessageDetailBodyTop + 10 - state.messagePage.messageScrollOffset,
               _width - _layout.padding * 2 - 24,
               bodyHeight + state.messagePage.messageScrollOffset + 80,
               true);

    const int footerY = _layout.bodyBottom - kMessageDetailFooterHeight;
    _display->drawRect(_layout.padding, footerY, _width - _layout.padding * 2, kMessageDetailFooterHeight, TFT_BLACK);
    drawUiText("返回", _layout.padding + 14, footerY + 10, 48, 20, false);
    drawUiText("上滑 / 下滑滚动消息详情", _layout.padding + 74, footerY + 10, _width - _layout.padding * 2 - 88, 20, false);
}

void PaperS3EDCRenderer::drawSettingsPage(const AppState& state) {
    char line[96];
    drawUiText("设备配置", _layout.padding, _layout.bodyTop, 120, 24, false);
    snprintf(line, sizeof(line), "网络：%s", state.config.wifiConfigured ? state.config.ssid : "未配置");
    drawUiText(line, _layout.padding, _layout.bodyTop + 28, _width - _layout.padding * 2, 24, false);
    snprintf(line, sizeof(line), "同步：%s", state.config.lastSyncLabel);
    drawUiText(line, _layout.padding, _layout.bodyTop + 54, _width - _layout.padding * 2, 24, false);

    for (int index = 0; index < kSettingsRowCount; ++index) {
        const int y = _layout.settingsRowRect(index).y;
        const bool active = (index == state.focusedSettingsIndex);
        _display->drawRect(_layout.padding, y, _width - _layout.padding * 2, 42, TFT_BLACK);
        if (active) {
            _display->fillRect(_layout.padding + 1, y + 1, _width - _layout.padding * 2 - 2, 40, TFT_BLACK);
            _display->setTextColor(TFT_WHITE, TFT_BLACK);
        }
        if (index == 0) {
            snprintf(line, sizeof(line), "插电拉取间隔  %u 秒", state.pollIntervalChargingSec);
        } else if (index == 1) {
            snprintf(line, sizeof(line), "电池拉取间隔  %u 秒", state.pollIntervalBatterySec);
        } else if (index == 2) {
            snprintf(line, sizeof(line), "Wi‑Fi 配置");
        } else if (index == 3) {
            snprintf(line, sizeof(line), "手动同步");
        } else {
            snprintf(line, sizeof(line), "返回今日");
        }
        drawUiText(line, _layout.padding + 14, y + 8, _width - _layout.padding * 2 - 28, 24, false);
        _display->setTextColor(TFT_BLACK, TFT_WHITE);
    }
}

void PaperS3EDCRenderer::drawWifiListPage(const AppState& state) {
    drawUiText("选择一个网络", _layout.padding, _layout.bodyTop, 140, 24, false);
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
            drawUiText("重新扫描中…", _layout.padding + 12, y + 8, 180, 24, false);
        } else {
            drawUiText(state.wifiNetworks[index].ssid, _layout.padding + 12, y + 8, _width - _layout.padding * 2 - 110, 24, false);
            _display->setCursor(_width - 92, y + 11);
            _display->printf("%ddBm", state.wifiNetworks[index].rssi);
        }
        _display->setTextColor(TFT_BLACK, TFT_WHITE);
    }
}

void PaperS3EDCRenderer::drawWifiPasswordPage(const AppState& state) {
    char line[96];
    snprintf(line, sizeof(line), "网络：%s", state.config.ssid[0] ? state.config.ssid : "未选择");
    drawUiText(line, _layout.padding, _layout.bodyTop, _width - _layout.padding * 2, 24, false);
    _display->drawRect(_layout.padding, _layout.bodyTop + 42, _width - _layout.padding * 2, 44, TFT_BLACK);
    if (state.passwordBuffer[0]) {
        drawUiText(state.passwordBuffer, _layout.padding + 12, _layout.bodyTop + 52, _width - _layout.padding * 2 - 24, 24, false);
    } else {
        drawUiText("请输入密码", _layout.padding + 12, _layout.bodyTop + 52, 160, 24, false);
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
