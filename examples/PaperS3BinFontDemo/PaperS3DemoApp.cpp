#include "PaperS3DemoApp.h"

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <M5GFX.h>
#include <M5Unified.h>
#include <SD.h>
#include <SPI.h>

#include <BinFontLib.h>

namespace {

constexpr int kHeaderHeight = 56;
constexpr int kFooterHeight = 84;
constexpr int kPadding = 14;
constexpr int kButtonGap = 12;
constexpr uint32_t kTouchDebounceMs = 150;
constexpr uint32_t kBenchmarkIterations = 10;
constexpr size_t kBenchmarkCaseCount = 5;

constexpr const char* kFontCandidates[] = {
    "/霞鹜文楷32.bin",
    "/fonts/霞鹜文楷32.bin",
    "/font/霞鹜文楷32.bin",
};

constexpr const char* kPreviewTitle = "PaperS3 BinFont Demo";
constexpr const char* kPreviewText =
    "你好，霞鹜文楷。\n"
    "这是 PaperS3 真机字体验证页面。\n"
    "目标：确认 bin 字体可加载、可显示、可测速。";
constexpr const char* kGlyphSample = "霞鹜文楷你好世界测试中文渲染";

constexpr const char* kBenchmarkText20 = "字体速度验证现在开始欢迎测试使用";
constexpr const char* kBenchmarkText48 =
    "这是第二档文字量测试页面，用接近五十字的中文句子来检查中短文本渲染速度与缓存效果。";
constexpr const char* kBenchmarkText96 =
    "第三档测试会把文本量提升到接近一百字，模拟菜单说明、章节介绍和状态提示等常见界面内容。"
    "我们继续观察平均耗时、单轮最大耗时，以及字形缓存是否已经充分建立。";
constexpr const char* kBenchmarkText180 =
    "第四档测试进入长段落渲染，文本会明显多于前面几页，用来模拟阅读器说明、剧情对白、任务描述和帮助页面。"
    "这种场景更容易暴露自动换行、整段排版、字形查表和缓存回收带来的额外成本，因此更接近闭源 PaperS3 项目的真实工作负载。";
constexpr const char* kBenchmarkTextFull =
    "第五档测试会尽量接近满屏文字显示。我们把多段中文连续排在同一页面，用来模拟电子纸设备在长文本场景下的真实刷新压力。"
    "在这一档里，单次渲染会涉及更多的字符查找、位图解码、区域绘制和自动换行处理，因此最能体现字体格式设计和缓存策略是否有效。"
    "如果平均耗时依旧稳定，说明当前 bin 字体加载与渲染路径已经具备较好的工程可行性，可以继续向正式项目移植。"
    "本页同时也帮助检查基线、段间距、边界裁剪和整页观感，确保不仅速度更快，而且在视觉上也可接受。";

enum class Page : uint8_t {
    Preview = 0,
    Glyphs,
    Benchmark,
};

enum class StorageKind : uint8_t {
    None = 0,
    LittleFS,
    SD,
};

struct FontSource {
    StorageKind kind = StorageKind::None;
    const char* label = "none";
    const char* path = nullptr;
};

struct BenchmarkSummary {
    uint32_t iterations = 0;
    uint32_t totalUs = 0;
    uint32_t averageUs = 0;
    uint32_t maxUs = 0;
    uint32_t charCount = 0;
    const char* label = "";
    RenderStats lastStats{};
};

struct BenchmarkCase {
    const char* label;
    const char* text;
};

M5FontPlatform g_platform(&LittleFS);
BinFontRuntime g_runtime(&g_platform);
M5FontRenderer g_renderer(&g_runtime, &M5.Display);

BenchmarkCase g_benchmarkCases[kBenchmarkCaseCount] = {
    {"Case1", kBenchmarkText20},
    {"Case2", kBenchmarkText48},
    {"Case3", kBenchmarkText96},
    {"Case4", kBenchmarkText180},
    {"Case5", kBenchmarkTextFull},
};
BenchmarkSummary g_benchmarkResults[kBenchmarkCaseCount];

Page g_page = Page::Benchmark;
FontSource g_fontSource;
bool g_littleFsReady = false;
bool g_sdReady = false;
bool g_fontLoaded = false;
bool g_touchActive = false;
bool g_benchmarkRan = false;
uint32_t g_lastTouchMs = 0;
int g_screenWidth = 0;
int g_screenHeight = 0;
String g_statusLine1;
String g_statusLine2;

const char* storageName(StorageKind kind) {
    switch (kind) {
        case StorageKind::LittleFS: return "LittleFS";
        case StorageKind::SD: return "SD";
        default: return "None";
    }
}

uint32_t countUtf8Chars(const char* text) {
    if (!text) return 0;
    const char* cursor = text;
    uint32_t count = 0;
    while (*cursor) {
        if (!utf8DecodeNext(cursor)) {
            break;
        }
        ++count;
    }
    return count;
}

void setStatus(const String& line1, const String& line2 = "") {
    g_statusLine1 = line1;
    g_statusLine2 = line2;
    Serial.println(line1);
    if (!line2.isEmpty()) {
        Serial.println(line2);
    }
}

void configureDisplay() {
    M5.Display.setRotation(1);
    if (M5.Display.isEPD()) {
        M5.Display.setEpdMode(epd_mode_t::epd_text);
    }
    M5.Display.fillScreen(TFT_WHITE);
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextSize(1);
    g_screenWidth = M5.Display.width();
    g_screenHeight = M5.Display.height();
}

void drawButton(int x, int y, int w, int h, const char* label, bool active) {
    M5.Display.drawRect(x, y, w, h, TFT_BLACK);
    M5.Display.fillRect(x + 1, y + 1, w - 2, h - 2, active ? TFT_BLACK : TFT_WHITE);
    M5.Display.setTextColor(active ? TFT_WHITE : TFT_BLACK, active ? TFT_BLACK : TFT_WHITE);
    M5.Display.setFont(&fonts::Font4);
    const int textWidth = M5.Display.textWidth(label);
    const int textX = x + (w - textWidth) / 2;
    const int textY = y + (h - 26) / 2;
    M5.Display.setCursor(textX, textY);
    M5.Display.print(label);
    M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
    M5.Display.setFont(&fonts::Font2);
}

void drawHeader(const char* title) {
    M5.Display.fillRect(0, 0, g_screenWidth, kHeaderHeight, TFT_WHITE);
    M5.Display.drawLine(0, kHeaderHeight - 1, g_screenWidth, kHeaderHeight - 1, TFT_BLACK);
    M5.Display.setFont(&fonts::Font4);
    M5.Display.setCursor(kPadding, 14);
    M5.Display.print(title);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setCursor(g_screenWidth - 240, 18);
    M5.Display.printf("Storage: %s", storageName(g_fontSource.kind));
}

void drawFooter() {
    const int y = g_screenHeight - kFooterHeight;
    const int innerY = y + 10;
    const int buttonWidth = (g_screenWidth - (kPadding * 2) - (kButtonGap * 2)) / 3;

    M5.Display.fillRect(0, y, g_screenWidth, kFooterHeight, TFT_WHITE);
    M5.Display.drawLine(0, y, g_screenWidth, y, TFT_BLACK);

    drawButton(kPadding, innerY, buttonWidth, 36, "Preview", g_page == Page::Preview);
    drawButton(kPadding + buttonWidth + kButtonGap, innerY, buttonWidth, 36, "Glyphs", g_page == Page::Glyphs);
    drawButton(kPadding + (buttonWidth + kButtonGap) * 2, innerY, buttonWidth, 36, "Bench", g_page == Page::Benchmark);

    M5.Display.setCursor(kPadding, y + 56);
    M5.Display.print(g_statusLine1);
    if (!g_statusLine2.isEmpty()) {
        M5.Display.setCursor(g_screenWidth / 2, y + 56);
        M5.Display.print(g_statusLine2);
    }
}

void showErrorScreen(const char* title, const char* detail) {
    M5.Display.fillScreen(TFT_WHITE);
    drawHeader(title);
    M5.Display.setFont(&fonts::Font4);
    M5.Display.setCursor(kPadding, kHeaderHeight + 24);
    M5.Display.print("Font load failed");
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setCursor(kPadding, kHeaderHeight + 70);
    M5.Display.print(detail);
    drawFooter();
    M5.Display.display();
}

bool mountStorage() {
    g_littleFsReady = LittleFS.begin(true);
    Serial.printf("[fs] LittleFS: %s\n", g_littleFsReady ? "ready" : "failed");

    SPI.begin(39, 40, 38, 47);
    g_sdReady = SD.begin(47, SPI, 25000000);
    Serial.printf("[fs] SD: %s\n", g_sdReady ? "ready" : "failed");

    if (!g_littleFsReady && !g_sdReady) {
        setStatus("No storage mounted", "LittleFS and SD unavailable");
        return false;
    }
    return true;
}

bool loadFontFrom(fs::FS& filesystem, StorageKind kind, const char* label, const char* path) {
    g_platform.setFilesystem(&filesystem);
    Serial.printf("[font] try %s:%s\n", label, path);
    if (!filesystem.exists(path)) {
        return false;
    }

    g_renderer.clearCaches();
    g_runtime.unload();
    if (!g_runtime.loadFont(path)) {
        Serial.printf("[font] load failed %s:%s\n", label, path);
        return false;
    }

    g_fontSource.kind = kind;
    g_fontSource.label = label;
    g_fontSource.path = path;
    g_fontLoaded = true;
    const FontHeader& header = g_runtime.getHeader();
    Serial.printf("[font] loaded %s:%s count=%u height=%u\n", label, path, header.char_count, header.font_height);
    setStatus(String("Font: ") + path, String("Chars: ") + header.char_count + " Height: " + header.font_height);
    return true;
}

bool loadFont() {
    for (const char* path : kFontCandidates) {
        if (g_littleFsReady && loadFontFrom(LittleFS, StorageKind::LittleFS, "LittleFS", path)) {
            return true;
        }
        if (g_sdReady && loadFontFrom(SD, StorageKind::SD, "SD", path)) {
            return true;
        }
    }

    setStatus("Font not found", "Searched LittleFS and SD");
    return false;
}

void beginContent() {
    M5.Display.fillRect(0, kHeaderHeight, g_screenWidth, g_screenHeight - kHeaderHeight - kFooterHeight, TFT_WHITE);
    g_renderer.setTextColor(TFT_BLACK);
    g_renderer.setBackgroundColor(TFT_WHITE);
    g_renderer.setRenderMode(M5FontRenderer::RenderMode::Text);
}

void logGlyphMetrics() {
    const char* cursor = kGlyphSample;
    Serial.println("[glyph] sample widths:");
    while (*cursor) {
        const char* before = cursor;
        uint16_t codepoint = utf8DecodeNext(cursor);
        if (!codepoint) {
            break;
        }
        char buffer[8] = {0};
        const size_t len = (size_t)(cursor - before);
        memcpy(buffer, before, len > sizeof(buffer) - 1 ? sizeof(buffer) - 1 : len);
        Serial.printf("  glyph=%s cp=U+%04X width=%d\n", buffer, codepoint, g_runtime.getCharWidth(codepoint));
    }
}

void renderPreviewPage() {
    beginContent();
    drawHeader("Preview");
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setCursor(kPadding, kHeaderHeight + 12);
    M5.Display.print("Real bin font preview using root asset");

    g_renderer.drawText(kPreviewTitle, kPadding, kHeaderHeight + 48, g_screenWidth - kPadding * 2, 48, true);
    RenderStats stats = g_renderer.drawText(kPreviewText, kPadding, kHeaderHeight + 110, g_screenWidth - kPadding * 2, 220, true);
    setStatus(String("Render ") + stats.render_us + " us", String(storageName(g_fontSource.kind)) + ":" + g_fontSource.path);
    drawFooter();
    M5.Display.display();
}

void renderGlyphPage() {
    beginContent();
    drawHeader("Glyph Check");
    M5.Display.setCursor(kPadding, kHeaderHeight + 12);
    M5.Display.print("Visual check for spacing, clipping, and baseline");

    RenderStats stats = g_renderer.drawText(kGlyphSample, kPadding, kHeaderHeight + 70, g_screenWidth - kPadding * 2, 220, true);
    M5.Display.setCursor(kPadding, kHeaderHeight + 320);
    M5.Display.printf("glyph_found=%u missing=%u", stats.glyph_found, stats.glyph_missing);
    setStatus(String("Glyph render ") + stats.render_us + " us", "See serial for per-glyph widths");
    drawFooter();
    M5.Display.display();
    logGlyphMetrics();
}

void drawBenchmarkCaseScreen(size_t caseIndex) {
    beginContent();
    drawHeader("Benchmark Run");
    M5.Display.setCursor(kPadding, kHeaderHeight + 12);
    M5.Display.printf("Auto case %u/%u", (unsigned)(caseIndex + 1), (unsigned)kBenchmarkCaseCount);

    const BenchmarkCase& item = g_benchmarkCases[caseIndex];
    g_renderer.drawText(item.text, kPadding, kHeaderHeight + 48, g_screenWidth - kPadding * 2, g_screenHeight - kHeaderHeight - kFooterHeight - 56, true);
    setStatus(String(item.label) + " running", String(countUtf8Chars(item.text)) + " chars");
    drawFooter();
    M5.Display.display();
}

BenchmarkSummary measureBenchmarkCase(const BenchmarkCase& item) {
    BenchmarkSummary summary;
    summary.label = item.label;
    summary.charCount = countUtf8Chars(item.text);

    g_renderer.clearCaches();
    for (uint32_t index = 0; index < kBenchmarkIterations; ++index) {
        M5.Display.fillRect(kPadding, kHeaderHeight + 48, g_screenWidth - kPadding * 2, g_screenHeight - kHeaderHeight - kFooterHeight - 56, TFT_WHITE);
        RenderStats stats = g_renderer.drawText(item.text, kPadding, kHeaderHeight + 48, g_screenWidth - kPadding * 2, g_screenHeight - kHeaderHeight - kFooterHeight - 56, true);
        summary.iterations++;
        summary.totalUs += stats.render_us;
        if (stats.render_us > summary.maxUs) {
            summary.maxUs = stats.render_us;
        }
        summary.lastStats = stats;
    }

    if (summary.iterations > 0) {
        summary.averageUs = summary.totalUs / summary.iterations;
    }
    return summary;
}

void printBenchmarkSummary() {
    Serial.println("[bench-suite] begin");
    for (size_t i = 0; i < kBenchmarkCaseCount; ++i) {
        const BenchmarkSummary& item = g_benchmarkResults[i];
        Serial.printf(
            "[bench-suite] %s chars=%u iterations=%u avg=%u max=%u total=%u glyph_found=%u bmp_hit=%u dec_hit=%u\n",
            item.label,
            item.charCount,
            item.iterations,
            item.averageUs,
            item.maxUs,
            item.totalUs,
            item.lastStats.glyph_found,
            item.lastStats.bmp_cache_hit,
            item.lastStats.dec_cache_hit
        );
    }
    Serial.println("[bench-suite] end");
}

void drawBenchmarkSummaryPage() {
    beginContent();
    drawHeader("Benchmark Summary");
    M5.Display.setCursor(kPadding, kHeaderHeight + 12);
    M5.Display.printf("5 cases x %u iterations", (unsigned)kBenchmarkIterations);

    int y = kHeaderHeight + 46;
    for (size_t i = 0; i < kBenchmarkCaseCount; ++i) {
        const BenchmarkSummary& item = g_benchmarkResults[i];
        M5.Display.setCursor(kPadding, y);
        M5.Display.printf("%s %3u chars avg %6u us", item.label, item.charCount, item.averageUs);
        y += 28;
        M5.Display.setCursor(kPadding + 12, y);
        M5.Display.printf("max %6u us glyph %3u", item.maxUs, item.lastStats.glyph_found);
        y += 28;
    }

    setStatus("Auto benchmark complete", "Check serial for full results");
    drawFooter();
    M5.Display.display();
}

void runBenchmarkSuite() {
    Serial.printf("[boot] auto benchmark enabled: cases=%u iterations=%u\n", (unsigned)kBenchmarkCaseCount, (unsigned)kBenchmarkIterations);
    for (size_t i = 0; i < kBenchmarkCaseCount; ++i) {
        drawBenchmarkCaseScreen(i);
        g_benchmarkResults[i] = measureBenchmarkCase(g_benchmarkCases[i]);
        Serial.printf(
            "[bench-case] %s chars=%u avg=%u max=%u total=%u\n",
            g_benchmarkResults[i].label,
            g_benchmarkResults[i].charCount,
            g_benchmarkResults[i].averageUs,
            g_benchmarkResults[i].maxUs,
            g_benchmarkResults[i].totalUs
        );
    }
    g_benchmarkRan = true;
    printBenchmarkSummary();
    drawBenchmarkSummaryPage();
}

void renderCurrentPage() {
    if (!g_fontLoaded) {
        showErrorScreen("Startup Error", "Font not loaded. Check uploadfs or SD card.");
        return;
    }

    switch (g_page) {
        case Page::Preview:
            renderPreviewPage();
            break;
        case Page::Glyphs:
            renderGlyphPage();
            break;
        case Page::Benchmark:
            if (!g_benchmarkRan) {
                runBenchmarkSuite();
            } else {
                drawBenchmarkSummaryPage();
            }
            break;
    }
}

void switchPage(Page page) {
    g_page = page;
    if (page != Page::Benchmark) {
        renderCurrentPage();
        return;
    }
    g_benchmarkRan = false;
    renderCurrentPage();
}

void handleTouch(int x, int y) {
    const int footerTop = g_screenHeight - kFooterHeight;
    const int buttonTop = footerTop + 10;
    const int buttonWidth = (g_screenWidth - (kPadding * 2) - (kButtonGap * 2)) / 3;
    const int buttonHeight = 36;
    if (y < buttonTop || y > buttonTop + buttonHeight) {
        return;
    }

    if (x >= kPadding && x <= kPadding + buttonWidth) {
        switchPage(Page::Preview);
        return;
    }

    const int glyphX = kPadding + buttonWidth + kButtonGap;
    if (x >= glyphX && x <= glyphX + buttonWidth) {
        switchPage(Page::Glyphs);
        return;
    }

    const int benchmarkX = glyphX + buttonWidth + kButtonGap;
    if (x >= benchmarkX && x <= benchmarkX + buttonWidth) {
        switchPage(Page::Benchmark);
    }
}

}  // namespace

void PaperS3Demo_setup() {
    auto config = M5.config();
    M5.begin(config);
    Serial.begin(115200);
    delay(400);

    configureDisplay();
    setStatus("Booting PaperS3 demo", "Preparing storage and font runtime");
    drawHeader("Startup");
    drawFooter();
    M5.Display.display();

    mountStorage();
    if (!loadFont()) {
        showErrorScreen("Font Missing", "Upload data/霞鹜文楷32.bin or place it on SD root.");
        return;
    }

    renderCurrentPage();
}

void PaperS3Demo_loop() {
    M5.update();

    auto touch = M5.Touch.getDetail();
    const bool pressed = touch.isPressed();
    if (pressed) {
        const uint32_t now = millis();
        if (!g_touchActive && (now - g_lastTouchMs) >= kTouchDebounceMs) {
            g_lastTouchMs = now;
            handleTouch(touch.x, touch.y);
        }
        g_touchActive = true;
    } else {
        g_touchActive = false;
    }

    delay(20);
}
