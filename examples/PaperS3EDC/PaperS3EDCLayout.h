#pragma once

#include <Arduino.h>

namespace papers3edc {

struct RectBox {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    constexpr RectBox() = default;
    constexpr RectBox(int px, int py, int pw, int ph) : x(px), y(py), w(pw), h(ph) {}

    bool contains(int px, int py) const {
        return px >= x && px <= (x + w) && py >= y && py <= (y + h);
    }
};

struct LayoutMetrics {
    int width = 0;
    int height = 0;

    int padding = 18;
    int statusHeight = 54;
    int footerHeight = 66;
    int footerButtonGap = 8;
    int footerButtonHeight = 36;
    int bodyTop = 74;
    int bodyBottom = 0;
    int footerTop = 0;

    int taskRowHeight = 58;
    int taskGap = 10;
    int taskListTop = 144;

    int settingsTop = 156;
    int settingsRowHeight = 58;

    int wifiRowTop = 118;
    int wifiRowHeight = 52;

    int keyboardTop = 222;
    int keyboardKeyW = 42;
    int keyboardKeyH = 42;
    int keyboardGap = 8;

    int confirmCardHeight = 250;
    int confirmButtonHeight = 46;
    int confirmButtonGap = 14;

    static LayoutMetrics portrait(int width, int height) {
        LayoutMetrics metrics;
        metrics.width = width;
        metrics.height = height;
        metrics.footerTop = height - metrics.footerHeight;
        metrics.bodyBottom = metrics.footerTop - 18;
        return metrics;
    }

    int footerButtonWidth() const {
        return (width - padding * 2 - footerButtonGap * 3) / 4;
    }

    RectBox footerButtonRect(int index) const {
        const int buttonWidth = footerButtonWidth();
        return RectBox{
            padding + index * (buttonWidth + footerButtonGap),
            footerTop + 10,
            buttonWidth,
            footerButtonHeight,
        };
    }

    RectBox taskRowRect(int index) const {
        return RectBox{
            padding,
            taskListTop + index * (taskRowHeight + taskGap),
            width - padding * 2,
            taskRowHeight,
        };
    }

    RectBox settingsRowRect(int index) const {
        return RectBox{
            padding,
            settingsTop + index * settingsRowHeight,
            width - padding * 2,
            42,
        };
    }

    RectBox wifiRowRect(int index) const {
        return RectBox{
            padding,
            wifiRowTop + index * wifiRowHeight,
            width - padding * 2,
            40,
        };
    }

    RectBox confirmBackRect() const {
        const int buttonW = (width - padding * 2 - confirmButtonGap) / 2;
        return RectBox{padding, bodyTop + confirmCardHeight + 28, buttonW, confirmButtonHeight};
    }

    RectBox confirmDoneRect() const {
        const int buttonW = (width - padding * 2 - confirmButtonGap) / 2;
        return RectBox{padding + buttonW + confirmButtonGap, bodyTop + confirmCardHeight + 28, buttonW, confirmButtonHeight};
    }

    RectBox keyboardKeyRect(int row, int col) const {
        return RectBox{
            padding + col * (keyboardKeyW + keyboardGap),
            keyboardTop + row * (keyboardKeyH + keyboardGap),
            keyboardKeyW,
            keyboardKeyH,
        };
    }
};

}  // namespace papers3edc
