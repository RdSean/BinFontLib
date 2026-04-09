#pragma once

#include <FS.h>
#include <M5Unified.h>

#include <BinFontLib.h>

namespace papers3edc {

class PaperS3EDCFontManager {
public:
    static constexpr size_t kPreviewFontCount = 1;

    struct PreviewFontSlot {
        const char* label = nullptr;
        const char* sample = nullptr;
        bool ready = false;
        int fontHeight = 0;
        const char* loadedPath = nullptr;
        const char* loadedStorageName = "None";
    };

    explicit PaperS3EDCFontManager(lgfx::LGFXBase* display);

    bool load(bool littleFsReady, bool sdReady);
    bool isReady() const { return _ready; }
    const char* loadedPath() const { return _loadedPath; }
    const char* loadedStorageName() const { return _loadedStorageName; }
    size_t previewSlotCount() const { return kPreviewFontCount; }
    const PreviewFontSlot& previewSlot(size_t index) const { return _previewSlots[index]; }

    RenderStats drawText(const char* text, int x, int y, int width, int height, bool wrap = true);
    RenderStats drawCompactText(const char* text, int x, int y, int width, int height, bool wrap = true);
    RenderStats drawTextToCanvas(M5Canvas& canvas, const char* text, int x, int y, int width, int height, bool wrap = true);
    RenderStats drawCompactTextToCanvas(M5Canvas& canvas, const char* text, int x, int y, int width, int height, bool wrap = true);
    RenderStats drawPreviewText(size_t index, const char* text, int x, int y, int width, int height, bool wrap = true);
    int getFontHeight() const;
    int getLineHeight() const;
    int measureTextHeight(const char* text, int width, bool wrap = true) const;
    int getCompactFontHeight() const;
    int getCompactLineHeight() const;
    int measureCompactTextHeight(const char* text, int width, bool wrap = true) const;
    int getPreviewFontHeight(size_t index) const;
    void clearCaches();

private:
    bool tryLoad(fs::FS& filesystem, const char* storageName, const char* path);
    bool tryLoadCompact(fs::FS& filesystem, const char* storageName, const char* path);
    bool tryLoadPreviewSlot(size_t index, bool littleFsReady, bool sdReady);
    const char* previewPathForIndex(size_t index) const;
    int measureRuntimeTextHeight(const BinFontRuntime& runtime, bool ready, const char* text, int width, bool wrap) const;

    lgfx::LGFXBase* _display = nullptr;
    M5FontPlatform _platform;
    BinFontRuntime _runtime;
    M5FontRenderer _renderer;
    M5FontPlatform _compactPlatform;
    BinFontRuntime _compactRuntime;
    M5FontRenderer _compactRenderer;
    M5FontPlatform _previewPlatform;
    BinFontRuntime _previewRuntime;
    M5FontRenderer _previewRenderer;
    bool _ready = false;
    bool _compactReady = false;
    const char* _loadedPath = nullptr;
    const char* _loadedStorageName = "None";
    const char* _compactLoadedPath = nullptr;
    const char* _compactLoadedStorageName = "None";
    PreviewFontSlot _previewSlots[kPreviewFontCount]{};
};

}  // namespace papers3edc
