# PaperS3 EPD Mode Selector Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add a persisted EPD refresh mode selector to the PaperS3 settings page so the device can switch between `Text`, `Fast`, `Fastest`, and `Quality`.

**Architecture:** Extend the app model with an `EpdRefreshMode` enum and store it in `DeviceConfig`, persist it through `PaperS3EDCStorage`, surface it as a new settings row, and map the selected value to `M5.Display.setEpdMode(...)` in the renderer.

**Tech Stack:** Arduino, M5Unified, M5GFX/LovyanGFX, LittleFS

---

### Task 1: Add persisted EPD mode to app state

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCModel.h`

**Step 1: Add enum**

Add `EpdRefreshMode` with `Text`, `Fast`, `Fastest`, `Quality`.

**Step 2: Add config field**

Store the selected value in `DeviceConfig`.

**Step 3: Keep default stable**

Default to `Text`.

### Task 2: Persist EPD mode in LittleFS config

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCStorage.cpp`

**Step 1: Load config field**

Read `epdMode=` if present.

**Step 2: Save config field**

Write `epdMode=<int>` to config output.

**Step 3: Keep backward compatibility**

If the field is absent, keep the default `Text`.

### Task 3: Expose mode in settings UI

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Add label helper**

Render a short mode label string for the current config value.

**Step 2: Add one settings row**

Increase settings row count and show `刷新模式  <label>`.

**Step 3: Handle tap changes**

Use left half for previous mode and right half for next mode, then save config and re-render settings.

### Task 4: Apply selected mode in renderer

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Map config mode to hardware mode**

Convert `EpdRefreshMode` to `epd_mode_t`.

**Step 2: Remove hardcoded text mode**

Use current config mode in `applyEpdMode()`.

### Task 5: Build and flash

**Files:**
- None

**Step 1: Build**

Run: `pio run -e papers3-edc`

Expected: `SUCCESS`

**Step 2: Flash**

Run: `pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101`

Expected: `SUCCESS`
