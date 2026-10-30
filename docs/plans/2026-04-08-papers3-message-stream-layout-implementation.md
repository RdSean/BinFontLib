# PaperS3 Message Stream Layout Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace the current bordered fixed-height message cards with a borderless message stream that computes item height from content and uses a left-side round-start marker for conversation grouping.

**Architecture:** Remove the message detail page and make the message tab render a single scrollable stream. Keep grouping and height calculation inside the renderer so business logic only tracks messages and scroll offset, while the renderer decides whether a message starts a new round and how much vertical space it needs.

**Tech Stack:** Arduino, M5Unified, M5GFX, BinFontLib, existing PaperS3 message state and sync path.

---

### Task 1: Remove message detail page state and navigation

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCModel.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Write the failing check**

Run:
```bash
rg -n "MessageDetail|selectedMessageDetailIndex|messageScrollOffset.*detail|drawMessageDetailPage" examples/PaperS3EDC
```

Expected: Old detail-page symbols still exist.

**Step 2: Implement the minimal removal**

- Remove `AppPage::MessageDetail`
- Remove detail-only selection state
- Remove `drawMessageDetailPage()`
- Replace tap-to-open-detail behavior with message-page scrolling behavior

**Step 3: Run the check again**

Run the same `rg` command.

Expected: Old detail-page path is gone.

**Step 4: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

### Task 2: Add message layout measurement helpers

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Write the failing check**

Run:
```bash
rg -n "messageHeight|measureMessage|messageRound|groupStart|indent" examples/PaperS3EDC/PaperS3EDCRenderer.*
```

Expected: No dynamic message layout helpers exist yet.

**Step 2: Implement the minimal helpers**

- Add helper(s) to determine whether a message starts a new round
- Add helper(s) to calculate item height from title/body/time
- Add helper(s) to compute left content column and vertical spacing

**Step 3: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

### Task 3: Replace fixed cards with borderless message stream rendering

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Write the failing check**

Run:
```bash
rg -n "kMessageCardHeight|kMessageCardGap|drawRect\\(_layout.padding, y, _width - _layout.padding \\* 2, kMessageCardHeight" examples/PaperS3EDC/PaperS3EDCRenderer.cpp
```

Expected: Fixed-height bordered message card rendering still exists.

**Step 2: Implement the minimal stream rendering**

- Remove bordered message cards
- Render messages as a continuous flow
- Draw a short thick vertical marker only for the first message in a round
- Leave the same left gutter for follow-up messages in the same round
- Render title/body/time with content-driven spacing

**Step 3: Run the check again**

Run the same `rg` command.

Expected: Fixed card rendering is removed.

**Step 4: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

### Task 4: Update message scrolling behavior to work with stream layout

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Write the failing target**

- Message page should scroll the single stream, not enter detail view
- Scroll offset should move the stream viewport consistently for long content

**Step 2: Implement the minimal interaction update**

- Convert message page touch handling into stream scroll actions
- Remove tap-to-open-detail flow
- Keep the scrolling step modest and EPD-friendly

**Step 3: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

### Task 5: Update docs and verify on device

**Files:**
- Modify: `readme/PAPERS3_EDC.md`

**Step 1: Update docs**

- Describe the message page as a borderless stream
- Remove mention of a separate message detail page
- Mention round-start marker and grouped message flow

**Step 2: Hardware verification**

Run:
```bash
pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101
```

Expected: Device shows borderless grouped messages without text overflowing a fixed card.
