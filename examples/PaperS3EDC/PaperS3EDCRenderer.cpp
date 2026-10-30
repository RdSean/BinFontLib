#include "PaperS3EDCRenderer.h"

#include <algorithm>
#include <ctype.h>
#include <stdio.h>

namespace papers3edc {
namespace {
constexpr int kStatusHeight = 54;
constexpr int kTaskMarkerSize = 22;
constexpr int kSectionGap = 16;
constexpr int kTitleLineHeight = 38;
constexpr int kKeyboardRows = 4;
constexpr int kKeyboardCols = 10;
constexpr int kMessageTopGap = 6;
constexpr int kMessageRoundGap = 18;
constexpr int kMessageBottomGap = 10;
constexpr int kMessageContentGap = 8;
constexpr int kMessageMetaGap = 6;
constexpr int kMessageGutterWidth = 22;
constexpr int kMessageMarkerWidth = 6;
constexpr int kMessageMarkerHeight = 34;
constexpr int kMessageRoundWindowMinutes = 3;
constexpr int kStatusIconSize = 18;
constexpr int kStatusSyncSlotWidth = 24;
constexpr int kStatusTaskSlotWidth = 40;
constexpr int kStatusBatteryWidth = 30;
constexpr int kMessageTimeColumnWidth = 56;
constexpr int kMessageTimeGap = 8;
constexpr int kMessageTimeRightInset = 14;
constexpr const char* kTabLabels[] = {"今日", "成长", "消息", "配置"};
constexpr const char* kPowerOffLine1 = "压反应釜冷却至 40°C";
constexpr const char* kPowerOffLine2 = "气相回路压力归零";
constexpr int kSettingsRowCount = 9;
constexpr const char* kWeekdayCn[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};

bool sameText(const char* left, const char* right) {
    if (!left || !right) {
        return false;
    }
    return strcmp(left, right) == 0;
}

int parseMessageMinutes(const char* text) {
    if (!text || !text[0]) {
        return -1;
    }

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    if (sscanf(text, "%d-%d-%dT%d:%d", &year, &month, &day, &hour, &minute) == 5
        || sscanf(text, "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute) == 5) {
        return (((year * 12) + month) * 31 + day) * 24 * 60 + hour * 60 + minute;
    }

    if (isdigit((unsigned char)text[0]) && isdigit((unsigned char)text[1]) && text[2] == ':') {
        if (sscanf(text, "%d:%d", &hour, &minute) == 2) {
            return hour * 60 + minute;
        }
    }

    return -1;
}

void formatMessageTime(const char* text, char* out, size_t outSize) {
    if (!out || outSize == 0) return;
    out[0] = '\0';
    if (!text || !text[0]) {
        return;
    }

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    if (sscanf(text, "%d-%d-%dT%d:%d", &year, &month, &day, &hour, &minute) == 5
        || sscanf(text, "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute) == 5
        || sscanf(text, "%d:%d", &hour, &minute) == 2) {
        snprintf(out, outSize, "%02d:%02d", hour, minute);
        return;
    }

    snprintf(out, outSize, "%.5s", text);
}

const char* pageTitle(AppPage page) {
    switch (page) {
        case AppPage::Today: return "今日安排";
        case AppPage::Confirm: return "确认完成";
        case AppPage::Progress: return "成长进度";
        case AppPage::Messages: return "消息";
        case AppPage::Settings: return "设备配置";
        case AppPage::RefreshLab: return "刷新实验";
        case AppPage::WifiList: return "选择网络";
        case AppPage::WifiPassword: return "输入密码";
        case AppPage::SyncStatus: return "同步结果";
        default: return "PaperS3 EDC";
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

const char* epdModeLabel(EpdRefreshMode mode) {
    switch (mode) {
        case EpdRefreshMode::Fast: return "Fast";
        case EpdRefreshMode::Fastest: return "Fastest";
        case EpdRefreshMode::Quality: return "Quality";
        case EpdRefreshMode::Text:
        default:
            return "Text";
    }
}

const char* refreshLabDrawPathLabel(RefreshLabDrawPath path) {
    switch (path) {
        case RefreshLabDrawPath::Canvas: return "Canvas";
        case RefreshLabDrawPath::Direct:
        default:
            return "Direct";
    }
}

epd_mode_t epdModeValue(EpdRefreshMode mode) {
    switch (mode) {
        case EpdRefreshMode::Fast: return epd_mode_t::epd_fast;
        case EpdRefreshMode::Fastest: return epd_mode_t::epd_fastest;
        case EpdRefreshMode::Quality: return epd_mode_t::epd_quality;
        case EpdRefreshMode::Text:
        default:
            return epd_mode_t::epd_text;
    }
}

RectBox toPanelRegion(lgfx::LGFXBase* display, const RectBox& logicalRegion) {
    if (!display) {
        return logicalRegion;
    }

    auto panel = M5.Display.panel();
    if (!panel) {
        return logicalRegion;
    }

    const auto cfg = panel->config();
    uint_fast8_t rotation = display->getRotation() & 7;
    uint_fast8_t internalRotation = ((rotation + cfg.offset_rotation) & 3) | ((rotation & 4) ^ (cfg.offset_rotation & 4));

    uint_fast16_t xs = logicalRegion.x;
    uint_fast16_t ys = logicalRegion.y;
    uint_fast16_t xe = logicalRegion.x + logicalRegion.w - 1;
    uint_fast16_t ye = logicalRegion.y + logicalRegion.h - 1;

    if (internalRotation) {
        if (internalRotation & 1) {
            std::swap(xs, ys);
            std::swap(xe, ye);
        }
        uint_fast8_t rb = 1 << internalRotation;
        if (rb & 0b11000110) {
            std::swap(xs, xe);
            xs = cfg.panel_width - 1 - xs;
            xe = cfg.panel_width - 1 - xe;
        }
        if (rb & 0b10011100) {
            std::swap(ys, ye);
            ys = cfg.panel_height - 1 - ys;
            ye = cfg.panel_height - 1 - ye;
        }
    }

    return RectBox{
        static_cast<int>(std::min(xs, xe)),
        static_cast<int>(std::min(ys, ye)),
        static_cast<int>(std::max(xs, xe) - std::min(xs, xe) + 1),
        static_cast<int>(std::max(ys, ye) - std::min(ys, ye) + 1),
    };
}
}  // namespace

PaperS3EDCRenderer::PaperS3EDCRenderer(lgfx::LGFXBase* display, PaperS3EDCFontManager* fonts)
    : _display(display), _fonts(fonts), _messageViewportCanvas(&M5.Display) {}

void PaperS3EDCRenderer::begin() {
    _display->setRotation(0);
    _display->setTextColor(TFT_BLACK, TFT_WHITE);
    _display->setTextSize(1);
    _display->setFont(&fonts::Font2);
    _width = _display->width();
    _height = _display->height();
    _layout = LayoutMetrics::portrait(_width, _height);
}

void PaperS3EDCRenderer::applyEpdMode(EpdRefreshMode mode) {
    if (M5.Display.isEPD()) {
        M5.Display.setEpdMode(epdModeValue(mode));
    }
}

void PaperS3EDCRenderer::displayFrame(EpdRefreshMode mode) {
    applyEpdMode(mode);
    _display->display();
}

void PaperS3EDCRenderer::displayRegion(const RectBox& region, EpdRefreshMode mode) {
    applyEpdMode(mode);
    const RectBox panelRegion = toPanelRegion(_display, region);
    _display->display(panelRegion.x, panelRegion.y, panelRegion.w, panelRegion.h);
}

void PaperS3EDCRenderer::redrawStatusBarToBuffer(const AppState& state) {
    const RectBox statusRegion = statusBarRegion();
    _display->fillRect(statusRegion.x, statusRegion.y, statusRegion.w, statusRegion.h, TFT_WHITE);
    drawStatusBar(state);
}

void PaperS3EDCRenderer::redrawMessageViewportToBuffer(const AppState& state) {
    const RectBox viewport = messageViewportRegion();
    _display->fillRect(viewport.x, viewport.y, viewport.w, viewport.h, TFT_WHITE);
    drawMessagesPage(state);
}

void PaperS3EDCRenderer::renderStatusBarFast(const AppState& state) {
    redrawStatusBarToBuffer(state);
    displayRegion(statusBarRegion(), EpdRefreshMode::Fast);
}

void PaperS3EDCRenderer::renderMessagePageFast(const AppState& state) {
    redrawMessageViewportToBuffer(state);
    displayRegion(messageViewportRegion(), EpdRefreshMode::Fast);
    renderStatusBarFast(state);
}

RectBox PaperS3EDCRenderer::statusBarRegion() const {
    return RectBox{0, 0, _width, _layout.statusHeight};
}

RectBox PaperS3EDCRenderer::messageViewportRegion() const {
    return RectBox{0, _layout.bodyTop, _width, _layout.bodyBottom - _layout.bodyTop};
}

RectBox PaperS3EDCRenderer::footerRegion() const {
    return RectBox{0, _layout.footerTop, _width, _layout.height - _layout.footerTop};
}

RectBox PaperS3EDCRenderer::refreshLabStatusRect() const {
    return RectBox{_layout.padding, _layout.bodyTop + 68, _width - _layout.padding * 2, 76};
}

RectBox PaperS3EDCRenderer::refreshLabBodyRect() const {
    return RectBox{_layout.padding, _layout.bodyTop + 164, _width - _layout.padding * 2, 210};
}

RectBox PaperS3EDCRenderer::refreshLabToggleRect() const {
    return RectBox{_layout.padding, _layout.bodyTop + 392, _width - _layout.padding * 2, 42};
}

RectBox PaperS3EDCRenderer::refreshLabBackRect() const {
    return RectBox{_layout.padding, _layout.bodyTop + 444, _width - _layout.padding * 2, 42};
}

bool PaperS3EDCRenderer::ensureMessageViewportCanvas() {
    const RectBox viewport = messageViewportRegion();
    if (_messageViewportCanvas.width() == viewport.w && _messageViewportCanvas.height() == viewport.h && _messageViewportCanvas.getBuffer()) {
        return true;
    }
    _messageViewportCanvas.deleteSprite();
    return _messageViewportCanvas.createSprite(viewport.w, viewport.h);
}

bool PaperS3EDCRenderer::blitCanvasRegionToDisplay(M5Canvas& canvas, const RectBox& canvasRegion, const RectBox& displayRegion) {
    if (canvasRegion.w <= 0 || canvasRegion.h <= 0 || displayRegion.w <= 0 || displayRegion.h <= 0) {
        return false;
    }
    if (canvasRegion.w != displayRegion.w || canvasRegion.h != displayRegion.h) {
        return false;
    }

    std::unique_ptr<uint16_t[]> pixels(new (std::nothrow) uint16_t[static_cast<size_t>(canvasRegion.w) * static_cast<size_t>(canvasRegion.h)]);
    if (!pixels) {
        return false;
    }

    canvas.readRect(canvasRegion.x, canvasRegion.y, canvasRegion.w, canvasRegion.h, pixels.get());
    _display->pushImage(displayRegion.x, displayRegion.y, displayRegion.w, displayRegion.h, pixels.get());
    return true;
}

RectBox PaperS3EDCRenderer::measureMessageContentRect(const AppState& state, size_t displayIndex, const MessageLayout& layout, int viewportTopOffset) const {
    const MessageItem& item = messageForDisplay(state, displayIndex);
    const int contentTop = layout.top + (layout.roundStart ? kMessageRoundGap : kMessageTopGap);
    const int bodyWidth = std::max(24, layout.contentWidth - kMessageTimeColumnWidth - kMessageTimeGap - kMessageTimeRightInset);
    const int timeX = layout.contentX + bodyWidth + kMessageTimeGap;
    int bodyX = layout.contentX;
    int left = bodyX;
    int top = contentTop;
    int bottom = contentTop;

    if (!item.read) {
        left = std::min(left, layout.contentX);
        top = std::min(top, contentTop + 10);
        bottom = std::max(bottom, contentTop + 18);
        bodyX += 16;
    }

    const int bodyTextWidth = std::max(16, bodyWidth - (bodyX - layout.contentX));
    const int bodyHeight = measureMessageTextHeight(item.body, bodyTextWidth, true);
    const char* timeText = item.sent_at;
    char timeLabel[8];
    formatMessageTime(timeText, timeLabel, sizeof(timeLabel));
    const int timeHeight = measureCompactTextHeight(timeLabel, kMessageTimeColumnWidth, false);

    left = std::min(left, bodyX);
    if (layout.roundStart) {
        left = std::min(left, _layout.padding + (kMessageGutterWidth - kMessageMarkerWidth) / 2);
        top = std::min(top, layout.top + kMessageRoundGap + 2);
        bottom = std::max(bottom, layout.top + kMessageRoundGap + 2 + kMessageMarkerHeight);
    }

    top = std::min(top, contentTop);
    bottom = std::max(bottom, contentTop + std::max(bodyHeight, timeHeight));
    const int right = std::max(bodyX + bodyTextWidth, timeX + kMessageTimeColumnWidth);
    return RectBox{left, viewportTopOffset + top, std::max(1, right - left), std::max(1, bottom - top)};
}

int PaperS3EDCRenderer::measureMessagePageCountForDisplayCount(const AppState& state, size_t displayCount) const {
    if (displayCount == 0) {
        return 0;
    }

    const int viewportHeight = _layout.bodyBottom - _layout.bodyTop;
    int usedHeight = 0;
    int pageCount = 1;
    for (size_t displayIndex = 0; displayIndex < displayCount; ++displayIndex) {
        const int itemHeight = measureMessageHeight(messageForDisplay(state, displayIndex), isMessageRoundStart(state, displayIndex));
        const bool needNextPage = usedHeight > 0 && (usedHeight + itemHeight) > viewportHeight;
        if (needNextPage) {
            ++pageCount;
            usedHeight = 0;
        }
        usedHeight += itemHeight;
    }
    return pageCount;
}

PaperS3EDCRenderer::MessageAppendRefreshPlan PaperS3EDCRenderer::measureMessageAppendRefreshPlan(AppPage previousPage, size_t previousMessageCount, const AppState& currentState) const {
    MessageAppendRefreshPlan plan;
    if (currentState.currentPage != AppPage::Messages || previousPage != AppPage::Messages) {
        return plan;
    }
    if (currentState.messageCount <= previousMessageCount || currentState.messageCount == 0) {
        return plan;
    }

    const MessagePageMetrics currentMetrics = measureMessagePageMetrics(currentState, currentState.messagePage.currentPageIndex);
    if (currentMetrics.pageCount <= 0 || currentMetrics.currentPageIndex != (currentMetrics.pageCount - 1)) {
        return plan;
    }

    const int previousPageCount = measureMessagePageCountForDisplayCount(currentState, previousMessageCount);
    if (previousMessageCount > 0 && previousPageCount != currentMetrics.pageCount) {
        return plan;
    }

    const int firstNewDisplayIndex = static_cast<int>(previousMessageCount);
    if (firstNewDisplayIndex < currentMetrics.startDisplayIndex || firstNewDisplayIndex >= currentMetrics.endDisplayIndex) {
        return plan;
    }

    const RectBox viewport = messageViewportRegion();
    int cursorY = 0;
    int left = viewport.x + viewport.w;
    int top = viewport.y + viewport.h;
    int right = viewport.x;
    int bottom = viewport.y;

    for (int displayIndex = currentMetrics.startDisplayIndex; displayIndex < currentMetrics.endDisplayIndex; ++displayIndex) {
        const MessageLayout layout = measureMessageLayout(currentState, static_cast<size_t>(displayIndex), cursorY);
        cursorY = layout.top + layout.height;
        if (displayIndex < firstNewDisplayIndex) {
            continue;
        }
        const RectBox contentRect = measureMessageContentRect(currentState, static_cast<size_t>(displayIndex), layout, viewport.y);
        left = std::min(left, contentRect.x);
        top = std::min(top, contentRect.y);
        right = std::max(right, contentRect.x + contentRect.w);
        bottom = std::max(bottom, contentRect.y + contentRect.h);
    }

    if (left >= right || top >= bottom) {
        return plan;
    }

    plan.eligible = true;
    plan.dirtyRegion = RectBox{
        std::max(viewport.x, left),
        std::max(viewport.y, top),
        std::min(viewport.x + viewport.w, right) - std::max(viewport.x, left),
        std::min(viewport.y + viewport.h, bottom) - std::max(viewport.y, top),
    };
    return plan;
}

bool PaperS3EDCRenderer::renderMessageAppendFast(AppPage previousPage, size_t previousMessageCount, const AppState& currentState) {
    const MessageAppendRefreshPlan plan = measureMessageAppendRefreshPlan(previousPage, previousMessageCount, currentState);
    if (!plan.eligible || plan.dirtyRegion.w <= 0 || plan.dirtyRegion.h <= 0) {
        return false;
    }
    if (!ensureMessageViewportCanvas()) {
        return false;
    }

    drawMessagesPageToCanvas(currentState, _messageViewportCanvas);
    const RectBox viewport = messageViewportRegion();
    const RectBox canvasRegion{
        plan.dirtyRegion.x - viewport.x,
        plan.dirtyRegion.y - viewport.y,
        plan.dirtyRegion.w,
        plan.dirtyRegion.h,
    };
    if (!blitCanvasRegionToDisplay(_messageViewportCanvas, canvasRegion, plan.dirtyRegion)) {
        return false;
    }
    displayRegion(plan.dirtyRegion, EpdRefreshMode::Fast);
    renderStatusBarFast(currentState);
    return true;
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

void PaperS3EDCRenderer::drawCompactText(const char* text, int x, int y, int width, int height, bool wrap) {
    if (_fonts && _fonts->isReady()) {
        RenderStats stats = _fonts->drawCompactText(text, x, y, width, height, wrap);
        if (stats.glyph_found > 0) {
            return;
        }
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

int PaperS3EDCRenderer::measureMessageTextHeight(const char* text, int width, bool wrap) const {
    if (!text || !text[0] || width <= 0) {
        return 0;
    }

    if (_fonts) {
        const int measured = _fonts->measureTextHeight(text, width, wrap);
        if (measured > 0) {
            return measured;
        }
    }

    constexpr int kFallbackLineHeight = 16;
    if (!wrap) {
        return kFallbackLineHeight;
    }
    const int charsPerLine = std::max(1, width / 8);
    const int lines = std::max(1, (int)((strlen(text) + charsPerLine - 1) / charsPerLine));
    return lines * kFallbackLineHeight;
}

int PaperS3EDCRenderer::measureCompactTextHeight(const char* text, int width, bool wrap) const {
    if (!text || !text[0] || width <= 0) {
        return 0;
    }

    if (_fonts) {
        const int measured = _fonts->measureCompactTextHeight(text, width, wrap);
        if (measured > 0) {
            return measured;
        }
    }

    constexpr int kFallbackLineHeight = 16;
    if (!wrap) {
        return kFallbackLineHeight;
    }
    const int charsPerLine = std::max(1, width / 8);
    const int lines = std::max(1, (int)((strlen(text) + charsPerLine - 1) / charsPerLine));
    return lines * kFallbackLineHeight;
}

size_t PaperS3EDCRenderer::messageStorageIndexForDisplay(const AppState& state, size_t displayIndex) const {
    return state.messageCount - 1 - displayIndex;
}

const MessageItem& PaperS3EDCRenderer::messageForDisplay(const AppState& state, size_t displayIndex) const {
    return state.messages[messageStorageIndexForDisplay(state, displayIndex)];
}

bool PaperS3EDCRenderer::isMessageRoundStart(const AppState& state, size_t displayIndex) const {
    if (displayIndex == 0 || displayIndex >= state.messageCount) {
        return true;
    }

    const MessageItem& current = messageForDisplay(state, displayIndex);
    const MessageItem& previous = messageForDisplay(state, displayIndex - 1);

    const int currentMinutes = parseMessageMinutes(current.sent_at);
    const int previousMinutes = parseMessageMinutes(previous.sent_at);
    if (currentMinutes < 0 || previousMinutes < 0) {
        return true;
    }

    const int delta = currentMinutes - previousMinutes;
    return delta < 0 || delta > kMessageRoundWindowMinutes;
}

int PaperS3EDCRenderer::measureMessageHeight(const MessageItem& item, bool roundStart) const {
    const int contentWidth = _width - _layout.padding * 2 - kMessageGutterWidth;
    const int bodyWidth = std::max(24, contentWidth - kMessageTimeColumnWidth - kMessageTimeGap - kMessageTimeRightInset);
    const int bodyHeight = measureMessageTextHeight(item.body, bodyWidth, true);
    const int topGap = roundStart ? kMessageRoundGap : kMessageTopGap;
    return topGap + bodyHeight + kMessageBottomGap;
}

PaperS3EDCRenderer::MessageLayout PaperS3EDCRenderer::measureMessageLayout(const AppState& state, size_t displayIndex, int y) const {
    MessageLayout layout;
    layout.roundStart = isMessageRoundStart(state, displayIndex);
    layout.top = y;
    layout.contentX = _layout.padding + kMessageGutterWidth;
    layout.contentWidth = _width - layout.contentX - _layout.padding;
    layout.height = measureMessageHeight(messageForDisplay(state, displayIndex), layout.roundStart);
    return layout;
}

PaperS3EDCRenderer::MessagePageMetrics PaperS3EDCRenderer::measureMessagePageMetrics(const AppState& state, int requestedPageIndex) const {
    MessagePageMetrics metrics;
    if (state.messageCount == 0) {
        return metrics;
    }

    const int viewportHeight = _layout.bodyBottom - _layout.bodyTop;
    int usedHeight = 0;
    int pageIndex = 0;
    int pageStartDisplayIndex = 0;
    bool foundPage = false;
    for (size_t displayIndex = 0; displayIndex < state.messageCount; ++displayIndex) {
        const int itemHeight = measureMessageHeight(messageForDisplay(state, displayIndex), isMessageRoundStart(state, displayIndex));
        const bool needNextPage = usedHeight > 0 && (usedHeight + itemHeight) > viewportHeight;
        if (needNextPage) {
            if (pageIndex == requestedPageIndex) {
                metrics.startDisplayIndex = pageStartDisplayIndex;
                metrics.endDisplayIndex = static_cast<int>(displayIndex);
                foundPage = true;
            }
            ++pageIndex;
            pageStartDisplayIndex = static_cast<int>(displayIndex);
            usedHeight = 0;
        }
        usedHeight += itemHeight;
    }

    metrics.pageCount = pageIndex + 1;
    metrics.currentPageIndex = std::max(0, std::min(requestedPageIndex, metrics.pageCount - 1));

    if (!foundPage) {
        if (metrics.currentPageIndex == pageIndex) {
            metrics.startDisplayIndex = pageStartDisplayIndex;
            metrics.endDisplayIndex = static_cast<int>(state.messageCount);
        } else {
            return measureMessagePageMetrics(state, metrics.currentPageIndex);
        }
    }

    return metrics;
}

int PaperS3EDCRenderer::messagePageCount(const AppState& state) const {
    return measureMessagePageMetrics(state, state.messagePage.currentPageIndex).pageCount;
}

int PaperS3EDCRenderer::clampMessagePageIndex(const AppState& state, int requestedPageIndex) const {
    return measureMessagePageMetrics(state, requestedPageIndex).currentPageIndex;
}

void PaperS3EDCRenderer::render(const AppState& state, RefreshMode mode) {
    switch (mode) {
        case RefreshMode::StatusOnly:
            redrawStatusBarToBuffer(state);
            displayFrame(EpdRefreshMode::Fast);
            return;
        case RefreshMode::MessagePage:
        case RefreshMode::MessageAppend:
            if (state.currentPage == AppPage::Messages) {
                redrawStatusBarToBuffer(state);
                redrawMessageViewportToBuffer(state);
                displayFrame(EpdRefreshMode::Text);
                return;
            }
            break;
        case RefreshMode::Full:
            break;
    }

    _display->fillScreen(TFT_WHITE);
    drawStatusBar(state);
    switch (state.currentPage) {
        case AppPage::Today: drawTodayPage(state); break;
        case AppPage::Confirm: drawConfirmPage(state); break;
        case AppPage::Progress: drawProgressPage(state); break;
        case AppPage::Messages: drawMessagesPage(state); break;
        case AppPage::Settings: drawSettingsPage(state); break;
        case AppPage::RefreshLab: drawRefreshLabPage(state); break;
        case AppPage::WifiList: drawWifiListPage(state); break;
        case AppPage::WifiPassword: drawWifiPasswordPage(state); break;
        case AppPage::SyncStatus: drawSyncStatusPage(state); break;
    }
    drawBottomTabs(state);
    displayFrame(EpdRefreshMode::Text);
}

void PaperS3EDCRenderer::renderPowerOffSplash() {
    _display->fillScreen(TFT_WHITE);

    const int contentWidth = _width - _layout.padding * 2;
    const int lineGap = 22;
    const int line1Height = std::max(40, measureMessageTextHeight(kPowerOffLine1, contentWidth, false));
    const int line2Height = std::max(40, measureMessageTextHeight(kPowerOffLine2, contentWidth, false));
    const int totalHeight = line1Height + lineGap + line2Height;
    const int startY = std::max(0, (_height - totalHeight) / 2);

    drawUiTextCentered(kPowerOffLine1, _layout.padding, startY, contentWidth, line1Height);
    drawUiTextCentered(kPowerOffLine2, _layout.padding, startY + line1Height + lineGap, contentWidth, line2Height);

    displayFrame(EpdRefreshMode::Quality);
}

void PaperS3EDCRenderer::drawStatusBar(const AppState& state) {
    _display->fillRect(0, 0, _width, _layout.statusHeight, TFT_WHITE);
    _display->drawLine(0, _layout.statusHeight - 1, _width, _layout.statusHeight - 1, TFT_BLACK);
    const int batteryX = _width - _layout.padding - kStatusBatteryWidth - 4;
    const int batteryY = 10;
    const int taskX = batteryX - kStatusTaskSlotWidth - 8;
    const int syncX = taskX - kStatusSyncSlotWidth - 8;
    const int titleWidth = std::max(96, syncX - _layout.padding - 8);

    char titleLabel[32];
    if (state.currentPage == AppPage::Messages && state.messageCount > 0) {
        const MessagePageMetrics pageMetrics = measureMessagePageMetrics(state, state.messagePage.currentPageIndex);
        snprintf(titleLabel, sizeof(titleLabel), "%s %d/%d", pageTitle(state.currentPage), pageMetrics.currentPageIndex + 1, pageMetrics.pageCount);
    } else {
        snprintf(titleLabel, sizeof(titleLabel), "%s", pageTitle(state.currentPage));
    }
    drawCompactText(titleLabel, _layout.padding, 12, titleWidth, 24, false);
    drawStatusSyncIcon(syncX, 12, kStatusIconSize, state.syncState);
    drawBatteryBar(batteryX, batteryY, kStatusBatteryWidth, 18, state.summary.batteryPercent, state.summary.charging);

    char progress[16];
    snprintf(progress, sizeof(progress), "%u/%u", state.summary.completedTasks, state.summary.totalTasks);
    drawCompactText(progress, taskX, 12, kStatusTaskSlotWidth, 20, false);
}

void PaperS3EDCRenderer::drawBottomTabs(const AppState& state) {
    const int y = _layout.footerTop;
    _display->drawLine(0, y, _width, y, TFT_BLACK);
    const int buttonWidth = _layout.footerButtonWidth();
    for (int index = 0; index < 4; ++index) {
        const int x = _layout.footerButtonRect(index).x;
        const bool active = ((index == 0 && state.currentPage == AppPage::Today)
                          || (index == 1 && state.currentPage == AppPage::Progress)
                          || (index == 2 && state.currentPage == AppPage::Messages)
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
    if (state.messageCount == 0) {
        const RectBox viewport = messageViewportRegion();
        _display->fillRect(viewport.x, viewport.y, viewport.w, viewport.h, TFT_WHITE);
        return;
    }

    if (ensureMessageViewportCanvas()) {
        drawMessagesPageToCanvas(state, _messageViewportCanvas);
        const RectBox viewport = messageViewportRegion();
        _messageViewportCanvas.pushSprite(viewport.x, viewport.y);
        return;
    }

    const int viewportTop = _layout.bodyTop;
    const int viewportBottom = _layout.bodyBottom;
    const MessagePageMetrics pageMetrics = measureMessagePageMetrics(state, state.messagePage.currentPageIndex);
    int cursorY = viewportTop;

    _display->setClipRect(0, viewportTop, _width, viewportBottom - viewportTop);

    for (int displayIndex = pageMetrics.startDisplayIndex; displayIndex < pageMetrics.endDisplayIndex; ++displayIndex) {
        const MessageItem& item = messageForDisplay(state, static_cast<size_t>(displayIndex));
        const MessageLayout layout = measureMessageLayout(state, static_cast<size_t>(displayIndex), cursorY);
        cursorY = layout.top + layout.height;

        if (layout.roundStart) {
            const int markerX = _layout.padding + (kMessageGutterWidth - kMessageMarkerWidth) / 2;
            const int markerY = layout.top + kMessageRoundGap + 2;
            _display->fillRect(markerX, markerY, kMessageMarkerWidth, kMessageMarkerHeight, TFT_BLACK);
        }

        const int contentTop = layout.top + (layout.roundStart ? kMessageRoundGap : kMessageTopGap);
        const int bodyWidth = std::max(24, layout.contentWidth - kMessageTimeColumnWidth - kMessageTimeGap - kMessageTimeRightInset);
        const int timeX = layout.contentX + bodyWidth + kMessageTimeGap;
        int bodyX = layout.contentX;
        if (!item.read) {
            _display->fillRect(layout.contentX, contentTop + 10, 8, 8, TFT_BLACK);
            bodyX += 16;
        }
        const int bodyTextWidth = std::max(16, bodyWidth - (bodyX - layout.contentX));
        char timeLabel[8];
        formatMessageTime(item.sent_at, timeLabel, sizeof(timeLabel));
        drawCompactText(timeLabel, timeX, contentTop, kMessageTimeColumnWidth, measureCompactTextHeight(timeLabel, kMessageTimeColumnWidth, false), false);

        const int bodyHeight = measureMessageTextHeight(item.body, bodyTextWidth, true);
        drawUiText(item.body, bodyX, contentTop, bodyTextWidth, bodyHeight, true);
    }

    _display->clearClipRect();
}

void PaperS3EDCRenderer::drawMessagesPageToCanvas(const AppState& state, M5Canvas& canvas) {
    if (state.messageCount == 0) {
        canvas.fillSprite(TFT_WHITE);
        return;
    }

    const int viewportHeight = canvas.height();
    const MessagePageMetrics pageMetrics = measureMessagePageMetrics(state, state.messagePage.currentPageIndex);
    int cursorY = 0;

    canvas.fillSprite(TFT_WHITE);

    for (int displayIndex = pageMetrics.startDisplayIndex; displayIndex < pageMetrics.endDisplayIndex; ++displayIndex) {
        const MessageItem& item = messageForDisplay(state, static_cast<size_t>(displayIndex));
        const MessageLayout layout = measureMessageLayout(state, static_cast<size_t>(displayIndex), cursorY);
        cursorY = layout.top + layout.height;
        if (layout.top >= viewportHeight) {
            break;
        }

        if (layout.roundStart) {
            const int markerX = _layout.padding + (kMessageGutterWidth - kMessageMarkerWidth) / 2;
            const int markerY = layout.top + kMessageRoundGap + 2;
            canvas.fillRect(markerX, markerY, kMessageMarkerWidth, kMessageMarkerHeight, TFT_BLACK);
        }

        const int contentTop = layout.top + (layout.roundStart ? kMessageRoundGap : kMessageTopGap);
        const int bodyWidth = std::max(24, layout.contentWidth - kMessageTimeColumnWidth - kMessageTimeGap - kMessageTimeRightInset);
        const int timeX = layout.contentX + bodyWidth + kMessageTimeGap;
        int bodyX = layout.contentX;
        if (!item.read) {
            canvas.fillRect(layout.contentX, contentTop + 10, 8, 8, TFT_BLACK);
            bodyX += 16;
        }
        const int bodyTextWidth = std::max(16, bodyWidth - (bodyX - layout.contentX));
        char timeLabel[8];
        formatMessageTime(item.sent_at, timeLabel, sizeof(timeLabel));
        if (_fonts && _fonts->isReady()) {
            _fonts->drawCompactTextToCanvas(canvas, timeLabel, timeX, contentTop, kMessageTimeColumnWidth, measureCompactTextHeight(timeLabel, kMessageTimeColumnWidth, false), false);
            const int bodyHeight = measureMessageTextHeight(item.body, bodyTextWidth, true);
            _fonts->drawTextToCanvas(canvas, item.body, bodyX, contentTop, bodyTextWidth, bodyHeight, true);
        } else {
            canvas.setFont(&fonts::Font2);
            canvas.setTextColor(TFT_BLACK, TFT_WHITE);
            canvas.setCursor(timeX, contentTop + 14);
            canvas.print(timeLabel);
            canvas.setCursor(bodyX, contentTop + 14);
            canvas.print(item.body);
        }
    }
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
            snprintf(line, sizeof(line), "刷新模式  %s", epdModeLabel(state.config.epdRefreshMode));
        } else if (index == 3) {
            snprintf(line, sizeof(line), "Wi‑Fi 配置");
        } else if (index == 4) {
            snprintf(line, sizeof(line), "手动同步");
        } else if (index == 5) {
            snprintf(line, sizeof(line), "刷新实验");
        } else if (index == 6) {
            snprintf(line, sizeof(line), "清空消息缓存");
        } else if (index == 7) {
            snprintf(line, sizeof(line), "关机");
        } else {
            snprintf(line, sizeof(line), "返回今日");
        }
        drawUiText(line, _layout.padding + 14, y + 8, _width - _layout.padding * 2 - 28, 24, false);
        _display->setTextColor(TFT_BLACK, TFT_WHITE);
    }
}

void PaperS3EDCRenderer::drawRefreshLabPage(const AppState& state) {
    char line[96];
    snprintf(line, sizeof(line), "模式：%s", epdModeLabel(state.config.epdRefreshMode));
    drawUiText(line, _layout.padding, _layout.bodyTop, _width - _layout.padding * 2, 24, false);
    snprintf(line, sizeof(line), "路径：%s", refreshLabDrawPathLabel(state.refreshLab.drawPath));
    drawUiText(line, _layout.padding, _layout.bodyTop + 28, _width - _layout.padding * 2, 24, false);

    RectBox statusRect = refreshLabStatusRect();
    RectBox bodyRect = refreshLabBodyRect();
    RectBox toggleRect = refreshLabToggleRect();
    RectBox backRect = refreshLabBackRect();

    _display->drawRect(statusRect.x, statusRect.y, statusRect.w, statusRect.h, TFT_BLACK);
    snprintf(line, sizeof(line), "顶栏局刷测试 #%u", state.refreshLab.statusCounter);
    drawUiText(line, statusRect.x + 14, statusRect.y + 10, statusRect.w - 28, 24, false);
    drawUiText("点击这里：只刷新顶部小区域", statusRect.x + 14, statusRect.y + 38, statusRect.w - 28, 24, false);

    _display->drawRect(bodyRect.x, bodyRect.y, bodyRect.w, bodyRect.h, TFT_BLACK);
    snprintf(line, sizeof(line), "正文局刷测试 #%u", state.refreshLab.bodyCounter);
    drawUiText(line, bodyRect.x + 14, bodyRect.y + 12, bodyRect.w - 28, 24, false);
    drawUiText("点击这里：只刷新消息区大小的矩形", bodyRect.x + 14, bodyRect.y + 44, bodyRect.w - 28, 24, false);
    drawUiText("观察残影、马赛克、丢内容和边界问题", bodyRect.x + 14, bodyRect.y + 76, bodyRect.w - 28, 48, true);

    _display->drawRect(toggleRect.x, toggleRect.y, toggleRect.w, toggleRect.h, TFT_BLACK);
    snprintf(line, sizeof(line), "切换绘制路径  %s", refreshLabDrawPathLabel(state.refreshLab.drawPath));
    drawUiText(line, toggleRect.x + 14, toggleRect.y + 8, toggleRect.w - 28, 24, false);

    _display->drawRect(backRect.x, backRect.y, backRect.w, backRect.h, TFT_BLACK);
    drawUiText("返回设置", backRect.x + 14, backRect.y + 8, backRect.w - 28, 24, false);
}

void PaperS3EDCRenderer::drawRefreshLabRegionDirect(const AppState& state, const RectBox& rect, bool bodyRegion, uint16_t counter) {
    char line[96];
    _display->fillRect(rect.x, rect.y, rect.w, rect.h, TFT_WHITE);
    _display->drawRect(rect.x, rect.y, rect.w, rect.h, TFT_BLACK);
    snprintf(line, sizeof(line), "%s #%u", bodyRegion ? "Body Direct" : "Status Direct", counter);
    drawUiText(line, rect.x + 14, rect.y + 10, rect.w - 28, 24, false);
    snprintf(line, sizeof(line), "Mode %s", epdModeLabel(state.config.epdRefreshMode));
    drawUiText(line, rect.x + 14, rect.y + 40, rect.w - 28, 24, false);
    if (bodyRegion) {
        drawUiText("这是固定正文矩形区域，用来测试局部提交是否稳定。", rect.x + 14, rect.y + 76, rect.w - 28, rect.h - 92, true);
    }
}

void PaperS3EDCRenderer::drawRefreshLabRegionToCanvas(const AppState& state, M5Canvas& canvas, const RectBox& rect, bool bodyRegion, uint16_t counter) {
    char line[96];
    canvas.fillSprite(TFT_WHITE);
    canvas.drawRect(0, 0, rect.w, rect.h, TFT_BLACK);
    snprintf(line, sizeof(line), "%s #%u", bodyRegion ? "Body Canvas" : "Status Canvas", counter);
    if (_fonts && _fonts->isReady()) {
        _fonts->drawCompactTextToCanvas(canvas, line, 14, 10, rect.w - 28, 24, false);
    } else {
        canvas.setTextColor(TFT_BLACK, TFT_WHITE);
        canvas.setFont(&fonts::Font2);
        canvas.setCursor(14, 20);
        canvas.print(line);
    }
    snprintf(line, sizeof(line), "Mode %s", epdModeLabel(state.config.epdRefreshMode));
    if (_fonts && _fonts->isReady()) {
        _fonts->drawCompactTextToCanvas(canvas, line, 14, 40, rect.w - 28, 24, false);
    } else {
        canvas.setCursor(14, 48);
        canvas.print(line);
    }
    if (bodyRegion) {
        const char* bodyText = "这是固定正文矩形区域，用来测试局部提交是否稳定。";
        if (_fonts && _fonts->isReady()) {
            _fonts->drawTextToCanvas(canvas, bodyText, 14, 76, rect.w - 28, rect.h - 92, true);
        } else {
            canvas.setCursor(14, 84);
            canvas.print("Canvas path partial refresh test.");
        }
    }
}

void PaperS3EDCRenderer::runRefreshLabStatusTest(const AppState& state) {
    const RectBox rect = refreshLabStatusRect();
    if (state.refreshLab.drawPath == RefreshLabDrawPath::Canvas) {
        M5Canvas canvas(&M5.Display);
        if (canvas.createSprite(rect.w, rect.h)) {
            drawRefreshLabRegionToCanvas(state, canvas, rect, false, state.refreshLab.statusCounter);
            canvas.pushSprite(rect.x, rect.y);
            canvas.deleteSprite();
        } else {
            drawRefreshLabRegionDirect(state, rect, false, state.refreshLab.statusCounter);
        }
    } else {
        drawRefreshLabRegionDirect(state, rect, false, state.refreshLab.statusCounter);
    }
    displayRegion(rect, state.config.epdRefreshMode);
}

void PaperS3EDCRenderer::runRefreshLabBodyTest(const AppState& state) {
    const RectBox rect = refreshLabBodyRect();
    if (state.refreshLab.drawPath == RefreshLabDrawPath::Canvas) {
        M5Canvas canvas(&M5.Display);
        if (canvas.createSprite(rect.w, rect.h)) {
            drawRefreshLabRegionToCanvas(state, canvas, rect, true, state.refreshLab.bodyCounter);
            canvas.pushSprite(rect.x, rect.y);
            canvas.deleteSprite();
        } else {
            drawRefreshLabRegionDirect(state, rect, true, state.refreshLab.bodyCounter);
        }
    } else {
        drawRefreshLabRegionDirect(state, rect, true, state.refreshLab.bodyCounter);
    }
    displayRegion(rect, state.config.epdRefreshMode);
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

void PaperS3EDCRenderer::drawStatusSyncIcon(int x, int y, int size, SyncState state) {
    const int right = x + size;
    const int bottom = y + size;
    const int midX = x + size / 2;
    const int midY = y + size / 2;
    switch (state) {
        case SyncState::Offline:
            _display->drawCircle(midX, midY, 6, TFT_BLACK);
            _display->drawLine(x + 3, bottom - 3, right - 3, y + 3, TFT_BLACK);
            break;
        case SyncState::Pending:
            _display->drawCircle(midX, midY, 6, TFT_BLACK);
            _display->fillTriangle(right - 6, y + 4, right - 1, y + 5, right - 4, y + 9, TFT_BLACK);
            _display->fillTriangle(x + 6, bottom - 4, x + 1, bottom - 5, x + 4, bottom - 9, TFT_BLACK);
            break;
        case SyncState::Failed:
            _display->drawTriangle(midX, y + 2, x + 2, bottom - 2, right - 2, bottom - 2, TFT_BLACK);
            _display->drawFastVLine(midX, y + 6, 5, TFT_BLACK);
            _display->fillRect(midX - 1, bottom - 6, 3, 3, TFT_BLACK);
            break;
        case SyncState::Success:
        case SyncState::Idle:
        default:
            _display->drawCircle(midX, midY, 6, TFT_BLACK);
            _display->fillCircle(midX, midY, 2, TFT_BLACK);
            break;
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
    drawUiText(title, _layout.padding + 18, _layout.bodyTop + 18, _width - _layout.padding * 2 - 36, 32, false);
    drawUiText(detail, _layout.padding + 18, _layout.bodyTop + 64, _width - _layout.padding * 2 - 36, 48, true);
}

}  // namespace papers3edc
