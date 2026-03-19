#pragma once

#include <M5Unified.h>

#include "PaperS3EDCFontManager.h"
#include "PaperS3EDCLayout.h"
#include "PaperS3EDCModel.h"

namespace papers3edc {

class PaperS3EDCRenderer {
public:
    PaperS3EDCRenderer(lgfx::LGFXBase* display, PaperS3EDCFontManager* fonts);

    void begin();
    void render(const AppState& state);

private:
    void applyEpdMode();
    void displayFrame();
    void drawStatusBar(const AppState& state);
    void drawBottomTabs(const AppState& state);
    void drawTaskMarker(int x, int y, int w, int h, TaskStatus status, bool selected);
    void drawTodayPage(const AppState& state);
    void drawConfirmPage(const AppState& state);
    void drawProgressPage(const AppState& state);
    void drawScreensaverPage(const AppState& state);
    void drawSettingsPage(const AppState& state);
    void drawWifiListPage(const AppState& state);
    void drawWifiPasswordPage(const AppState& state);
    void drawSyncStatusPage(const AppState& state);
    void drawBatteryBar(int x, int y, int width, int height, int32_t percent, bool charging);
    void drawNotionLineArt(int x, int y, int w, int h);
    void drawKeyboardGrid(const AppState& state, int x, int y, int keyW, int keyH, int gap);
    void drawFallbackBodyMessage(const char* title, const char* detail);

    lgfx::LGFXBase* _display = nullptr;
    PaperS3EDCFontManager* _fonts = nullptr;
    int _width = 0;
    int _height = 0;
    LayoutMetrics _layout{};
};

}  // namespace papers3edc
