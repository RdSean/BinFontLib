# PaperS3 Message Minimal Layout Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Simplify each message item to a minimal time-plus-body layout, using 32 pt for message body text and 28 pt for status/time text.

**Architecture:** Keep the existing message polling, storage, and scroll behavior unchanged while updating only the renderer and any supporting font-selection logic. Message rows will reserve a fixed right-side timestamp column and a left-side body column, removing sender and source metadata from the message-list presentation.

**Tech Stack:** Arduino, M5Unified/M5GFX, LittleFS-hosted font assets, existing PaperS3 EDC renderer/font manager modules

---

### Task 1: Inspect the current message metadata rendering and time layout

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCFontManager.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCFontManager.h`

**Step 1: Confirm sender/source rendering is still present**

Run:
```bash
rg -n "messageSender|item.source|sent_at|drawUiText\\(item.body" examples/PaperS3EDC/PaperS3EDCRenderer.cpp
```

Expected: sender/source/time/body are still rendered in the current message layout.

**Step 2: Inspect current font asset usage**

Run:
```bash
rg -n "32.bin|28.bin|霞鹜文楷32|LXGWWenKaiLite-for-edc" examples/PaperS3EDC/PaperS3EDCFontManager.cpp examples/PaperS3EDC/PaperS3EDCFontManager.h examples/PaperS3EDC
```

Expected: current font loading and selection points are visible for later updates.

### Task 2: Add explicit font usage for 28 pt status text and 32 pt message body text

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCFontManager.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCFontManager.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Write the failing inspection check**

Run:
```bash
rg -n "LXGWWenKaiLite-for-edc-28.bin|LXGWWenKaiLite-for-edc-32.bin" examples/PaperS3EDC/PaperS3EDCFontManager.cpp examples/PaperS3EDC/PaperS3EDCRenderer.cpp
```

Expected: the renderer is not yet explicitly split into 28 pt status/time usage and 32 pt message-body usage.

**Step 2: Implement or wire distinct font access paths**

Add the minimal support needed so:

- status-bar text and compact message time use the 28 pt font
- message body uses the 32 pt font

Keep the existing fallback behavior for cases where LittleFS fonts are unavailable.

**Step 3: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

### Task 3: Replace sender/source rows with fixed time column plus body column

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Write the failing inspection check**

Run:
```bash
rg -n "messageSender|item.source|metaLine|timeWidth" examples/PaperS3EDC/PaperS3EDCRenderer.cpp
```

Expected: old sender/source layout and the current time positioning are still present.

**Step 2: Implement the minimal message layout**

In `drawMessagesPage()` and the message-height helpers:

- remove sender-name rendering from the message list
- remove source-line rendering from the message list
- reserve a fixed-width right-side time column
- format message time into a compact display such as `08:14`
- wrap the body only within the remaining left-side width

**Step 3: Update height measurement to match the new layout**

Make sure message height is based on:

- top gap
- body height using the narrower body column
- bottom gap

Do not leave legacy title/meta height assumptions in the row-height calculation.

### Task 4: Verify clipping and spacing with the new larger body font

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Re-run text/layout checks**

Run:
```bash
rg -n "messageSender|item.source|metaLine" examples/PaperS3EDC/PaperS3EDCRenderer.cpp
```

Expected: sender/source list rendering paths are gone.

**Step 2: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

### Task 5: Flash and verify on device

**Files:**
- No code changes required if previous steps pass

**Step 1: Flash the updated firmware**

Run:
```bash
pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101
```

Expected: upload succeeds.

**Step 2: Verify message item appearance on hardware**

Expected:

- each message row shows only compact time plus body text
- no `relay://...` text appears
- no sender name appears
- time stays visually separated from long body text

**Step 3: Verify typography and clipping**

Expected:

- body text is visibly larger than status-bar text
- status-bar text remains compact
- long message bodies do not overlap the time column
- long message bodies still do not draw into the top status bar or bottom tab bar
