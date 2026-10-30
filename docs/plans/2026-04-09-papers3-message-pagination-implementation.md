# PaperS3 Message Pagination Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Convert the message page from a reverse-chronological scroll stream into a chronological paged reader with left/right navigation and top-bar page feedback.

**Architecture:** Keep message polling and storage intact where possible, but replace scroll-offset-based message viewing with an explicit pagination model derived from the current viewport height. Render only the current page, jump to the last page after new messages arrive, and surface page feedback in the top status bar.

**Tech Stack:** Arduino, M5Unified/M5GFX, LittleFS-backed custom fonts, existing PaperS3 EDC renderer/controller/storage modules

---

### Task 1: Inspect current message ordering, scroll state, and touch behavior

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCModel.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Confirm scroll-based message navigation is still present**

Run:
```bash
rg -n "messageScrollOffset|向上翻页|向下翻页|messageScrollMax" examples/PaperS3EDC/PaperS3EDCApp.cpp examples/PaperS3EDC/PaperS3EDCRenderer.cpp examples/PaperS3EDC/PaperS3EDCModel.h
```

Expected: message viewing still depends on scroll offset and vertical paging messages.

**Step 2: Confirm render order assumptions**

Run:
```bash
rg -n "messages\\[index\\]|mergeMessage|state.messageCount" examples/PaperS3EDC/PaperS3EDCSync.cpp examples/PaperS3EDC/PaperS3EDCRenderer.cpp
```

Expected: current merge/render logic shows where newest-first insertion happens.

### Task 2: Add failing structural checks for chronological pagination state

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCModel.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Run failing inspection checks**

Run:
```bash
rg -n "currentPageIndex|pageCount|PageIndicator|messagePageCount" examples/PaperS3EDC/PaperS3EDCModel.h examples/PaperS3EDC/PaperS3EDCRenderer.h examples/PaperS3EDC/PaperS3EDCRenderer.cpp
```

Expected: no explicit pagination state/helpers exist yet.

**Step 2: Define the minimal new state surface**

Add only the state needed for page-based reading, such as:

- current message page index
- derived page count accessors or page-metric helpers

Do not keep obsolete scroll-only state once the new approach is wired through.

### Task 3: Implement chronological page layout in the renderer

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Replace stream-height / scroll-max logic with page metrics**

Implement helpers that:

- iterate messages in chronological order
- group them into pages based on visible viewport height
- expose total page count
- expose the item range or y-positions for the current page

**Step 2: Render only the active page**

In `drawMessagesPage()`:

- stop using `messageScrollOffset`
- render only the selected page
- keep clipping active
- preserve compact time column and 32 pt body layout

**Step 3: Add message-page status indicator support**

Expose a compact page label for the status bar, such as `2/5`, for use only on the message page.

### Task 4: Update controller logic to use left/right page navigation and auto-jump

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCModel.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCStorage.cpp`

**Step 1: Replace message scroll interaction**

In message-page touch handling:

- left half of the message viewport goes to previous page
- right half goes to next page
- clamp to valid page bounds

**Step 2: Auto-jump to the last page on new messages**

When `pullMessages()` returns new content:

- recompute page bounds implicitly through renderer helpers
- set the current page to the last page
- re-render immediately if the message page is visible

**Step 3: Persist the new page state if needed**

If the app keeps message-page state across restarts, save/load page index rather than obsolete scroll offset.

If preserving the current page across restarts adds unnecessary complexity, reset to page 0 or last page explicitly and remove old persisted scroll state.

### Task 5: Move message navigation feedback into the top status bar

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Update status-bar composition**

When `state.currentPage == AppPage::Messages`:

- keep the single-row compact layout
- show message-page indicator in the title area or a reserved compact slot
- avoid reintroducing overflow

**Step 2: Remove obsolete footer-dependent navigation messaging**

Do not rely on footer text for page number feedback anymore.

### Task 6: Verify build and flash on hardware

**Files:**
- No code changes required if previous steps pass

**Step 1: Run static checks**

Run:
```bash
rg -n "messageScrollOffset|向上翻页|向下翻页" examples/PaperS3EDC/PaperS3EDCApp.cpp examples/PaperS3EDC/PaperS3EDCRenderer.cpp examples/PaperS3EDC/PaperS3EDCStorage.cpp examples/PaperS3EDC/PaperS3EDCModel.h
```

Expected: obsolete scroll-based message navigation paths are removed or no longer used.

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

**Step 4: Verify on device**

Expected:

- the first page starts with older messages
- newer messages appear on later pages
- left/right taps switch pages
- the top bar shows current page feedback
- receiving a new message moves the message page to the end
- content still stays within the message viewport without covering the status bar or bottom tabs
