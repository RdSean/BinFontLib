#include "PaperS3EDCFontManager.h"

#include <LittleFS.h>
#include <SD.h>

namespace papers3edc {
namespace {
constexpr const char* kFontCandidates[] = {
    "/霞鹜文楷32.bin",
    "/fonts/霞鹜文楷32.bin",
    "/font/霞鹜文楷32.bin",
};
}  // namespace

PaperS3EDCFontManager::PaperS3EDCFontManager(lgfx::LGFXBase* display)
    : _display(display), _platform(&LittleFS), _runtime(&_platform), _renderer(&_runtime, display) {
    _renderer.setRenderMode(M5FontRenderer::RenderMode::Text);
    _renderer.setTextColor(TFT_BLACK);
    _renderer.setBackgroundColor(TFT_WHITE);
}

bool PaperS3EDCFontManager::tryLoad(fs::FS& filesystem, const char* storageName, const char* path) {
    _platform.setFilesystem(&filesystem);
    if (!filesystem.exists(path)) {
        return false;
    }
    _renderer.clearCaches();
    _runtime.unload();
    if (!_runtime.loadFont(path)) {
        return false;
    }
    _ready = true;
    _loadedStorageName = storageName;
    _loadedPath = path;
    return true;
}

bool PaperS3EDCFontManager::load(bool littleFsReady, bool sdReady) {
    _ready = false;
    _loadedPath = nullptr;
    _loadedStorageName = "None";

    for (const char* path : kFontCandidates) {
        if (littleFsReady && tryLoad(LittleFS, "LittleFS", path)) {
            return true;
        }
        if (sdReady && tryLoad(SD, "SD", path)) {
            return true;
        }
    }
    return false;
}

RenderStats PaperS3EDCFontManager::drawText(const char* text, int x, int y, int width, int height, bool wrap) {
    if (!_ready) {
        return RenderStats{};
    }
    return _renderer.drawText(text, x, y, width, height, wrap);
}

int PaperS3EDCFontManager::getFontHeight() const {
    if (!_ready) {
        return 0;
    }
    return _runtime.getHeader().font_height;
}

void PaperS3EDCFontManager::clearCaches() {
    _renderer.clearCaches();
}

}  // namespace papers3edc
