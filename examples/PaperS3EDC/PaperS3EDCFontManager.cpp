#include "PaperS3EDCFontManager.h"

#include <algorithm>
#include <LittleFS.h>
#include <SD.h>

#include "core/BinFontRuntime.h"

namespace papers3edc {
namespace {
constexpr const char* kFontCandidates[] = {
    "/LXGWWenKaiLite-for-edc-32.bin",
    "/fonts/LXGWWenKaiLite-for-edc-32.bin",
    "/font/LXGWWenKaiLite-for-edc-32.bin",
};

constexpr const char* kCompactFontCandidates[] = {
    "/LXGWWenKaiLite-for-edc-28.bin",
    "/fonts/LXGWWenKaiLite-for-edc-28.bin",
    "/font/LXGWWenKaiLite-for-edc-28.bin",
};

struct PreviewFontSpec {
    const char* label;
    const char* sample;
    const char* paths[3];
};

constexpr PreviewFontSpec kPreviewFontSpecs[PaperS3EDCFontManager::kPreviewFontCount] = {
    {
        "Lite28",
        "LXGWWenKaiLite 28\n今天要完成的事\n钉钉消息：你好，凌日奇观",
        {"/LXGWWenKaiLite-for-edc-28.bin", "/fonts/LXGWWenKaiLite-for-edc-28.bin", "/font/LXGWWenKaiLite-for-edc-28.bin"},
    },
};
}  // namespace

PaperS3EDCFontManager::PaperS3EDCFontManager(lgfx::LGFXBase* display)
    : _display(display),
      _platform(&LittleFS),
      _runtime(&_platform),
      _renderer(&_runtime, display),
      _compactPlatform(&LittleFS),
      _compactRuntime(&_compactPlatform),
      _compactRenderer(&_compactRuntime, display),
      _previewPlatform(&LittleFS),
      _previewRuntime(&_previewPlatform),
      _previewRenderer(&_previewRuntime, display) {
    _renderer.setRenderMode(M5FontRenderer::RenderMode::Text);
    _renderer.setTextColor(TFT_BLACK);
    _renderer.setBackgroundColor(TFT_WHITE);
    _compactRenderer.setRenderMode(M5FontRenderer::RenderMode::Text);
    _compactRenderer.setTextColor(TFT_BLACK);
    _compactRenderer.setBackgroundColor(TFT_WHITE);
    _previewRenderer.setRenderMode(M5FontRenderer::RenderMode::Text);
    _previewRenderer.setTextColor(TFT_BLACK);
    _previewRenderer.setBackgroundColor(TFT_WHITE);
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

bool PaperS3EDCFontManager::tryLoadCompact(fs::FS& filesystem, const char* storageName, const char* path) {
    _compactPlatform.setFilesystem(&filesystem);
    if (!filesystem.exists(path)) {
        return false;
    }
    _compactRenderer.clearCaches();
    _compactRuntime.unload();
    if (!_compactRuntime.loadFont(path)) {
        return false;
    }
    _compactReady = true;
    _compactLoadedStorageName = storageName;
    _compactLoadedPath = path;
    return true;
}

bool PaperS3EDCFontManager::load(bool littleFsReady, bool sdReady) {
    _ready = false;
    _compactReady = false;
    _loadedPath = nullptr;
    _loadedStorageName = "None";
    _compactLoadedPath = nullptr;
    _compactLoadedStorageName = "None";
    for (size_t index = 0; index < kPreviewFontCount; ++index) {
        _previewSlots[index] = PreviewFontSlot{};
        _previewSlots[index].label = kPreviewFontSpecs[index].label;
        _previewSlots[index].sample = kPreviewFontSpecs[index].sample;
        _previewSlots[index].loadedStorageName = "None";
    }

    for (const char* path : kFontCandidates) {
        if (littleFsReady && tryLoad(LittleFS, "LittleFS", path)) {
            break;
        }
        if (sdReady && tryLoad(SD, "SD", path)) {
            break;
        }
    }
    for (const char* path : kCompactFontCandidates) {
        if (littleFsReady && tryLoadCompact(LittleFS, "LittleFS", path)) {
            break;
        }
        if (sdReady && tryLoadCompact(SD, "SD", path)) {
            break;
        }
    }
    for (size_t index = 0; index < kPreviewFontCount; ++index) {
        tryLoadPreviewSlot(index, littleFsReady, sdReady);
    }
    return _ready;
}

RenderStats PaperS3EDCFontManager::drawText(const char* text, int x, int y, int width, int height, bool wrap) {
    if (!_ready) {
        return RenderStats{};
    }
    return _renderer.drawText(text, x, y, width, height, wrap);
}

RenderStats PaperS3EDCFontManager::drawCompactText(const char* text, int x, int y, int width, int height, bool wrap) {
    if (!_compactReady) {
        return RenderStats{};
    }
    return _compactRenderer.drawText(text, x, y, width, height, wrap);
}

RenderStats PaperS3EDCFontManager::drawPreviewText(size_t index, const char* text, int x, int y, int width, int height, bool wrap) {
    if (index >= kPreviewFontCount || !_previewSlots[index].ready || !_previewSlots[index].loadedPath) {
        return RenderStats{};
    }

    fs::FS* filesystem = nullptr;
    if (strcmp(_previewSlots[index].loadedStorageName, "SD") == 0) {
        filesystem = &SD;
    } else {
        filesystem = &LittleFS;
    }
    _previewPlatform.setFilesystem(filesystem);
    _previewRenderer.clearCaches();
    _previewRuntime.unload();
    if (!_previewRuntime.loadFont(_previewSlots[index].loadedPath)) {
        return RenderStats{};
    }
    return _previewRenderer.drawText(text, x, y, width, height, wrap);
}

int PaperS3EDCFontManager::getFontHeight() const {
    if (!_ready) {
        return 0;
    }
    return _runtime.getHeader().font_height;
}

int PaperS3EDCFontManager::getLineHeight() const {
    if (!_ready) {
        return 0;
    }
    return _runtime.getLineAdvance();
}

int PaperS3EDCFontManager::getCompactFontHeight() const {
    if (!_compactReady) {
        return 0;
    }
    return _compactRuntime.getHeader().font_height;
}

int PaperS3EDCFontManager::getCompactLineHeight() const {
    if (!_compactReady) {
        return 0;
    }
    return _compactRuntime.getLineAdvance();
}

int PaperS3EDCFontManager::getPreviewFontHeight(size_t index) const {
    if (index >= kPreviewFontCount || !_previewSlots[index].ready) {
        return 0;
    }
    return _previewSlots[index].fontHeight;
}

int PaperS3EDCFontManager::measureTextHeight(const char* text, int width, bool wrap) const {
    return measureRuntimeTextHeight(_runtime, _ready, text, width, wrap);
}

int PaperS3EDCFontManager::measureCompactTextHeight(const char* text, int width, bool wrap) const {
    return measureRuntimeTextHeight(_compactRuntime, _compactReady, text, width, wrap);
}

int PaperS3EDCFontManager::measureRuntimeTextHeight(const BinFontRuntime& runtime, bool ready, const char* text, int width, bool wrap) const {
    if (!text || !text[0] || width <= 0) {
        return 0;
    }

    if (!ready) {
        constexpr int kFallbackCharWidth = 8;
        constexpr int kFallbackLineHeight = 16;
        if (!wrap) {
            return kFallbackLineHeight;
        }

        const size_t textLength = strlen(text);
        const int charsPerLine = std::max(1, width / kFallbackCharWidth);
        const int lines = std::max(1, (int)((textLength + charsPerLine - 1) / charsPerLine));
        return lines * kFallbackLineHeight;
    }

    const int fontHeight = std::max(1, static_cast<int>(runtime.getHeader().font_height));
    const int lineHeight = fontHeight;
    if (!wrap) {
        return lineHeight;
    }

    int lineCount = 0;
    const char* cursor = text;
    while (*cursor) {
        TextMeasure measure = measureText(const_cast<BinFontRuntime&>(runtime), cursor, width);
        if (measure.charCount == 0) {
            const char* next = cursor;
            if (utf8DecodeNext(next) == 0) {
                break;
            }
            cursor = next;
            ++lineCount;
            continue;
        }
        const char* next = cursor;
        for (size_t index = 0; index < measure.charCount; ++index) {
            if (utf8DecodeNext(next) == 0) {
                break;
            }
        }
        cursor = next;
        ++lineCount;
    }

    return std::max(1, lineCount) * lineHeight;
}

void PaperS3EDCFontManager::clearCaches() {
    _renderer.clearCaches();
    _compactRenderer.clearCaches();
    _previewRenderer.clearCaches();
}

const char* PaperS3EDCFontManager::previewPathForIndex(size_t index) const {
    if (index >= kPreviewFontCount) {
        return nullptr;
    }
    return _previewSlots[index].loadedPath ? _previewSlots[index].loadedPath : kPreviewFontSpecs[index].paths[0];
}

bool PaperS3EDCFontManager::tryLoadPreviewSlot(size_t index, bool littleFsReady, bool sdReady) {
    if (index >= kPreviewFontCount) {
        return false;
    }

    PreviewFontSlot& slot = _previewSlots[index];
    const PreviewFontSpec& spec = kPreviewFontSpecs[index];
    const char* candidatePaths[] = {spec.paths[0], spec.paths[1], spec.paths[2]};
    slot.ready = false;
    slot.fontHeight = 0;
    slot.loadedPath = nullptr;
    slot.loadedStorageName = "None";

    for (const char* path : candidatePaths) {
        if (!path) continue;

        if (littleFsReady) {
            _previewPlatform.setFilesystem(&LittleFS);
            if (LittleFS.exists(path)) {
                _previewRenderer.clearCaches();
                _previewRuntime.unload();
                if (_previewRuntime.loadFont(path)) {
                    slot.ready = true;
                    slot.fontHeight = _previewRuntime.getHeader().font_height;
                    slot.loadedStorageName = "LittleFS";
                    slot.loadedPath = path;
                    return true;
                }
            }
        }
        if (sdReady) {
            _previewPlatform.setFilesystem(&SD);
            if (SD.exists(path)) {
                _previewRenderer.clearCaches();
                _previewRuntime.unload();
                if (_previewRuntime.loadFont(path)) {
                    slot.ready = true;
                    slot.fontHeight = _previewRuntime.getHeader().font_height;
                    slot.loadedStorageName = "SD";
                    slot.loadedPath = path;
                    return true;
                }
            }
        }
    }

    return false;
}

}  // namespace papers3edc
