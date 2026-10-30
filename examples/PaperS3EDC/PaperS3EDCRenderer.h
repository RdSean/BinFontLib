#pragma once

#include <memory>

#include <M5Unified.h>

#include "PaperS3EDCFontManager.h"
#include "PaperS3EDCLayout.h"
#include "PaperS3EDCModel.h"

namespace papers3edc {

class PaperS3EDCRenderer {
public:
    PaperS3EDCRenderer(lgfx::LGFXBase* display, PaperS3EDCFontManager* fonts);

    void begin();
    void render(const AppState& state, RefreshMode mode = RefreshMode::Full);
    void renderPowerOffSplash();
    int messagePageCount(const AppState& state) const;
    int clampMessagePageIndex(const AppState& state, int requestedPageIndex) const;
    bool renderMessageAppendFast(AppPage previousPage, size_t previousMessageCount, const AppState& currentState);
    void renderMessagePageFast(const AppState& state);
    void renderStatusBarFast(const AppState& state);
    RectBox statusBarRegion() const;
    RectBox messageViewportRegion() const;
    RectBox footerRegion() const;
    RectBox refreshLabStatusRect() const;
    RectBox refreshLabBodyRect() const;
    RectBox refreshLabToggleRect() const;
    RectBox refreshLabBackRect() const;
    void runRefreshLabStatusTest(const AppState& state);
    void runRefreshLabBodyTest(const AppState& state);

private:
    struct MessageLayout {
        int top = 0;
        int height = 0;
        int contentX = 0;
        int contentWidth = 0;
        bool roundStart = false;
    };
    struct MessagePageMetrics {
        int pageCount = 0;
        int currentPageIndex = 0;
        int startDisplayIndex = 0;
        int endDisplayIndex = 0;
    };
    struct MessageAppendRefreshPlan {
        bool eligible = false;
        RectBox dirtyRegion{};
    };

    void applyEpdMode(EpdRefreshMode mode);
    void displayFrame(EpdRefreshMode mode);
    void displayRegion(const RectBox& region, EpdRefreshMode mode);
    bool ensureMessageViewportCanvas();
    bool blitCanvasRegionToDisplay(M5Canvas& canvas, const RectBox& canvasRegion, const RectBox& displayRegion);
    void redrawStatusBarToBuffer(const AppState& state);
    void redrawMessageViewportToBuffer(const AppState& state);
    void drawStatusBar(const AppState& state);
    void drawBottomTabs(const AppState& state);
    void drawTaskMarker(int x, int y, int w, int h, TaskStatus status, bool selected);
    void drawUiText(const char* text, int x, int y, int width, int height, bool wrap = false);
    void drawCompactText(const char* text, int x, int y, int width, int height, bool wrap = false);
    void drawUiTextCentered(const char* text, int x, int y, int width, int height);
    void drawTodayPage(const AppState& state);
    void drawConfirmPage(const AppState& state);
    void drawProgressPage(const AppState& state);
    void drawMessagesPage(const AppState& state);
    void drawMessagesPageToCanvas(const AppState& state, M5Canvas& canvas);
    void drawSettingsPage(const AppState& state);
    void drawRefreshLabPage(const AppState& state);
    void drawWifiListPage(const AppState& state);
    void drawWifiPasswordPage(const AppState& state);
    void drawSyncStatusPage(const AppState& state);
    void drawRefreshLabRegionDirect(const AppState& state, const RectBox& rect, bool bodyRegion, uint16_t counter);
    void drawRefreshLabRegionToCanvas(const AppState& state, M5Canvas& canvas, const RectBox& rect, bool bodyRegion, uint16_t counter);
    void drawStatusSyncIcon(int x, int y, int size, SyncState state);
    void drawBatteryBar(int x, int y, int width, int height, int32_t percent, bool charging);
    void drawNotionLineArt(int x, int y, int w, int h);
    void drawKeyboardGrid(const AppState& state, int x, int y, int keyW, int keyH, int gap);
    void drawFallbackBodyMessage(const char* title, const char* detail);
    size_t messageStorageIndexForDisplay(const AppState& state, size_t displayIndex) const;
    const MessageItem& messageForDisplay(const AppState& state, size_t displayIndex) const;
    bool isMessageRoundStart(const AppState& state, size_t displayIndex) const;
    int measureMessageTextHeight(const char* text, int width, bool wrap) const;
    int measureCompactTextHeight(const char* text, int width, bool wrap) const;
    int measureMessageHeight(const MessageItem& item, bool roundStart) const;
    MessageLayout measureMessageLayout(const AppState& state, size_t displayIndex, int y) const;
    MessagePageMetrics measureMessagePageMetrics(const AppState& state, int requestedPageIndex) const;
    RectBox measureMessageContentRect(const AppState& state, size_t displayIndex, const MessageLayout& layout, int viewportTopOffset) const;
    int measureMessagePageCountForDisplayCount(const AppState& state, size_t displayCount) const;
    MessageAppendRefreshPlan measureMessageAppendRefreshPlan(AppPage previousPage, size_t previousMessageCount, const AppState& currentState) const;

    lgfx::LGFXBase* _display = nullptr;
    PaperS3EDCFontManager* _fonts = nullptr;
    M5Canvas _messageViewportCanvas;
    int _width = 0;
    int _height = 0;
    LayoutMetrics _layout{};
};

}  // namespace papers3edc
