# PaperS3 Power-Off Splash Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 在硬件电源键长按 3 秒时，先显示两行关机屏保，再执行关机。

**Architecture:** 在应用主循环中监听 `M5.BtnPWR` 长按状态。首次达到阈值时调用 renderer 绘制关机屏保并做一次高质量全屏刷新，随后短延时并调用 `M5.Power.powerOff()`。渲染复用现有 BinFontLib 字体路径，不引入新的页面状态。

**Tech Stack:** M5Unified, M5GFX, BinFontLib, Arduino C++, PlatformIO

---

### Task 1: Add Renderer Support For Power-Off Splash

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Add a dedicated renderer entry**

Expose a method that draws the two fixed lines centered on a white screen and submits with `EpdRefreshMode::Quality`.

**Step 2: Use BinFontLib text drawing**

Render both lines with the existing main font path instead of fallback M5 fonts.

**Step 3: Keep implementation isolated**

Do not add a new `AppPage`; this is a transient overlay for shutdown only.

### Task 2: Trigger Splash On 3-Second Power Button Hold

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Track power-button hold state**

Add a boolean guard so the splash/power-off sequence runs only once per hold.

**Step 2: Detect 3-second hold**

In the main loop, after `M5.update()`, check `M5.BtnPWR.pressedFor(3000)` or equivalent hold state.

**Step 3: Render splash then power off**

Call the new renderer method, wait briefly so the EPD update can settle, then call `M5.Power.powerOff()`.

### Task 3: Verify Build And Flash

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Build firmware**

Run: `pio run -e papers3-edc`
Expected: `SUCCESS`

**Step 2: Upload firmware**

Run: `pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101`
Expected: `SUCCESS`

**Step 3: Manual verification**

Verify:
- 长按电源键约 3 秒后出现两行关机文案
- 文案居中且清晰
- 随后设备关机
