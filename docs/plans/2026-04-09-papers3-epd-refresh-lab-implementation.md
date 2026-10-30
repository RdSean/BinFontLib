# PaperS3 EPD Refresh Lab Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace the oversized font diagnostics area in settings with a dedicated EPD refresh lab entry and implement a minimal lab page for controlled region-refresh experiments.

**Architecture:** Remove the settings-page diagnostics block, add a new `RefreshLab` page plus a small transient lab state, and implement two region-refresh experiments that reuse the existing persisted EPD mode while switching between direct and canvas-backed drawing paths.

**Tech Stack:** Arduino, M5Unified, M5GFX/LovyanGFX, LittleFS

---

### Task 1: Extend the page and lab state model

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCModel.h`

**Step 1: Add a refresh lab page enum entry**

Add a dedicated app page for the EPD refresh lab.

**Step 2: Add transient lab state**

Store:

- draw path (`Direct` / `Canvas`)
- status test counter
- body test counter

### Task 2: Simplify the settings page

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Remove font diagnostics block**

Delete the large diagnostics drawing block below the settings rows.

**Step 2: Add refresh lab row label**

Show `刷新实验` as a normal settings row.

### Task 3: Add settings-page navigation

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Increase settings row handling**

Update touch handling so the new row opens the refresh lab page.

**Step 2: Preserve existing rows**

Keep interval, EPD mode, Wi-Fi, manual sync, and return behavior intact.

### Task 4: Implement the refresh lab page UI

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Add page title and renderer entry**

Render a dedicated refresh lab page.

**Step 2: Draw minimal experiment layout**

Include:

- mode label
- draw-path label
- status test rectangle
- body test rectangle
- toggle row
- back row

### Task 5: Implement refresh lab interactions

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Add direct and canvas region test helpers**

Implement small, isolated region-refresh helpers for:

- status rectangle
- body rectangle

**Step 2: Hook touch behavior**

- tapping the status test rect triggers one status-area region refresh
- tapping the body test rect triggers one body-area region refresh
- tapping toggle row switches draw path
- tapping back row returns to settings

### Task 6: Build and flash

**Files:**
- None

**Step 1: Build**

Run: `pio run -e papers3-edc`

Expected: `SUCCESS`

**Step 2: Flash**

Run: `pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101`

Expected: `SUCCESS`
