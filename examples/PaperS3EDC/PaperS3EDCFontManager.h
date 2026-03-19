#pragma once

#include <FS.h>
#include <M5Unified.h>

#include <BinFontLib.h>

namespace papers3edc {

class PaperS3EDCFontManager {
public:
    explicit PaperS3EDCFontManager(lgfx::LGFXBase* display);

    bool load(bool littleFsReady, bool sdReady);
    bool isReady() const { return _ready; }
    const char* loadedPath() const { return _loadedPath; }
    const char* loadedStorageName() const { return _loadedStorageName; }

    RenderStats drawText(const char* text, int x, int y, int width, int height, bool wrap = true);
    int getFontHeight() const;
    void clearCaches();

private:
    bool tryLoad(fs::FS& filesystem, const char* storageName, const char* path);

    lgfx::LGFXBase* _display = nullptr;
    M5FontPlatform _platform;
    BinFontRuntime _runtime;
    M5FontRenderer _renderer;
    bool _ready = false;
    const char* _loadedPath = nullptr;
    const char* _loadedStorageName = "None";
};

}  // namespace papers3edc
