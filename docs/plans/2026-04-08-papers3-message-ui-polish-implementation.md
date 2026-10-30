# PaperS3 Message UI Polish Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Clarify the automatic message refresh UI, fix top-bar alignment, and prevent message content from drawing outside the message viewport.

**Architecture:** Keep the existing message polling and touch interaction intact, and limit this change to renderer and page-state presentation. The main work is a top status bar relayout plus hard clipping of message stream drawing to the body viewport. No backend behavior or page structure changes are required.

**Tech Stack:** Arduino, M5Unified/M5GFX, existing PaperS3 EDC renderer/controller/layout modules

---

### Task 1: Inspect the current status bar and message viewport behavior

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Confirm the current top-bar text layout**

Run:
```bash
rg -n "消息收件箱|已同步|drawStatusBar|drawBatteryBar" examples/PaperS3EDC/PaperS3EDCRenderer.cpp
```

Expected: the message title, sync text, and battery/task widgets are all still drawn in the current one-line layout.

**Step 2: Confirm the current message drawing path**

Run:
```bash
rg -n "drawMessagesPage|viewportTop|viewportBottom|messageScrollOffset" examples/PaperS3EDC/PaperS3EDCRenderer.cpp examples/PaperS3EDC/PaperS3EDCApp.cpp
```

Expected: message scrolling exists, but there is no explicit clipping of the message viewport.

### Task 2: Add a compact icon-based sync indicator and relayout the status bar

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Write the failing inspection check**

Run:
```bash
rg -n "syncStateName\\(|消息收件箱" examples/PaperS3EDC/PaperS3EDCRenderer.cpp
```

Expected: text-based sync state and the long message title are still present.

**Step 2: Implement fixed-width right-side status slots**

Add renderer helpers to:

- draw a compact sync icon for `Idle`, `Pending`, `Offline`, and `Failed`
- reserve fixed horizontal space for sync icon, task count, and battery indicator
- shorten the message page title to `消息`

**Step 3: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

### Task 3: Clip message rendering to the body viewport

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Write the failing inspection check**

Run:
```bash
rg -n "startWrite|setClipRect|clearClipRect|endWrite" examples/PaperS3EDC/PaperS3EDCRenderer.cpp
```

Expected: no viewport clipping is used in the message page yet.

**Step 2: Implement viewport clipping for the message stream**

In `drawMessagesPage()`:

- keep the current viewport bounds based on `bodyTop` and `bodyBottom`
- clip all message-body drawing to that rectangle
- restore the clip state after message rendering

**Step 3: Preserve current touch scrolling behavior**

Do not change the existing scroll direction logic in `handleTouch()`. Only ensure the drawn content stays inside the viewport while scrolling.

**Step 4: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

### Task 4: Tighten page wording and footer copy for automatic refresh

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Remove manual-feeling wording where it conflicts with auto-refresh**

Adjust short labels so the UI no longer suggests the user must manually refresh messages in normal operation.

Targets include:

- page title wording
- message-page status text after auto polling
- any obvious relay-era phrasing that clashes with live auto refresh

**Step 2: Re-run text checks**

Run:
```bash
rg -n "消息收件箱|已同步|relay 已刷新" examples/PaperS3EDC/PaperS3EDCRenderer.cpp examples/PaperS3EDC/PaperS3EDCApp.cpp
```

Expected:

- `消息收件箱` is gone
- top-bar sync text is no longer used there
- old relay-oriented copy is removed or replaced

### Task 5: Verify on hardware with real auto polling

**Files:**
- No code changes required if previous steps pass

**Step 1: Flash the updated firmware**

Run:
```bash
pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101
```

Expected: upload succeeds.

**Step 2: Verify the top bar on device**

Expected:

- top bar remains one row high
- page title, sync icon, task count, and battery icon align without overlap
- the message page title is short enough to fit cleanly

**Step 3: Verify message clipping and auto refresh**

Expected:

- long message threads scroll without drawing into the status bar
- long message threads scroll without drawing into the bottom tab bar
- when new DingTalk messages arrive, the message page updates through automatic polling without requiring manual refresh
