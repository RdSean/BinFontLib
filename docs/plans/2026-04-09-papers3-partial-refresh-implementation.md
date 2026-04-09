# PaperS3 Partial Refresh Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add a region-based partial refresh architecture so PaperS3 can refresh the status bar and message viewport independently, keep the footer static during message updates, and periodically fall back to a full refresh.

**Architecture:** Introduce a refresh coordinator above the renderer. Keep a full-screen fallback path, but add explicit refresh intents and semantic regions. Use a canvas-backed render path for the message viewport, and a bounded status-bar refresh path for page/sync changes. Count partial refreshes and periodically trigger a full refresh.

**Tech Stack:** Arduino, M5Unified/M5GFX, M5Canvas/LGFX Sprite facilities, existing PaperS3 EDC renderer/controller/font stack

---

### Task 1: Inspect current full-screen EPD submission path

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`

**Step 1: Confirm current renderer is full-frame**

Run:
```bash
rg -n "setEpdMode|display\\(|setClipRect" examples/PaperS3EDC/PaperS3EDCRenderer.cpp
```

Expected: renderer clips drawing but still ends in a full `_display->display()`.

**Step 2: Confirm there is no PaperS3-specific canvas refresh layer**

Run:
```bash
rg -n "M5Canvas|LGFX_Sprite|drawTextToCanvas|partial refresh|dirty rect" examples/PaperS3EDC src
```

Expected: canvas support exists in shared font/platform code, but not yet in PaperS3 inbox rendering flow.

### Task 2: Add refresh intent and region abstractions

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCModel.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Add structural checks**

Run:
```bash
rg -n "RefreshMode|RefreshRegion|partialRefreshCount|MessageViewportRegion|StatusBarRegion" examples/PaperS3EDC
```

Expected: no refresh abstraction exists yet.

**Step 2: Define the minimal abstractions**

Add:

- refresh mode enum
- region descriptors or rect helpers for status bar / message viewport / footer
- partial refresh counters/state as needed

Keep the abstractions small and specific to current needs.

### Task 3: Implement canvas-backed message viewport rendering

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCFontManager.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCFontManager.cpp`

**Step 1: Add a message viewport render target path**

Create the minimal support needed to:

- allocate a canvas for the message viewport
- clear it to white
- render the current message page into that canvas
- push the canvas back to the message viewport rect

**Step 2: Keep the existing full-screen path intact**

Do not remove full render support. Partial refresh is additive, not a replacement.

### Task 4: Implement status-bar-only and message-page refresh paths

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Add refresh entry points**

Support at least:

- full refresh
- status-only refresh
- message-page refresh

**Step 2: Wire message-page interactions**

On left/right page navigation:

- refresh `StatusBarRegion`
- refresh `MessageViewportRegion`
- do not refresh `FooterRegion`

**Step 3: Wire message arrival**

When new messages arrive:

- update page state
- refresh `StatusBarRegion`
- refresh `MessageViewportRegion`
- do not refresh `FooterRegion`

### Task 5: Add periodic forced full refresh policy

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCModel.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Introduce partial-refresh counter state**

Track how many partial message/status refreshes have occurred since the last full refresh.

**Step 2: Add threshold policy**

Use a named constant and force a full refresh after the configured number of partial refreshes.

**Step 3: Reset counter on full refresh**

Make sure the counter is reset whenever a full-screen refresh path runs.

### Task 6: Verify build, flash, and device behavior

**Files:**
- No code changes required if previous steps pass

**Step 1: Static verification**

Run:
```bash
rg -n "M5Canvas|RefreshMode|partialRefreshCount|message viewport|status bar" examples/PaperS3EDC
```

Expected: new refresh architecture symbols are present.

**Step 2: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

**Step 3: Flash**

Run:
```bash
pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101
```

Expected: PASS.

**Step 4: Verify on hardware**

Expected:

- paging updates status bar + message viewport only
- footer does not blink on message page navigation
- new message arrival updates status bar + message viewport only
- after threshold partial refreshes, a full refresh occurs
- fallback full refresh path still renders correctly
