# PaperS3 DingTalk Messages Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace the screensaver tab with a read-only message inbox backed by a simple relay API, with configurable polling intervals and scrollable message viewing on PaperS3.

**Architecture:** Reuse the current PaperS3 EDC app structure, but replace screensaver-specific state and rendering with message list and message detail state. Keep message polling behind the sync boundary, persist the local cache and polling configuration, and keep renderer/font responsibilities separate from app logic.

**Tech Stack:** Arduino, M5Unified, M5GFX, LittleFS, BinFontLib, existing `PaperS3EDCSync`, existing `PaperS3EDCStorage`, PaperS3 touch input.

---

### Task 1: Replace screensaver navigation with message navigation

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCModel.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`
- Modify: `readme/PAPERS3_EDC.md`

**Step 1: Write the failing check**

Run:
```bash
rg -n "Screensaver|屏保" examples/PaperS3EDC/PaperS3EDCModel.h examples/PaperS3EDC/PaperS3EDCRenderer.h examples/PaperS3EDC/PaperS3EDCRenderer.cpp examples/PaperS3EDC/PaperS3EDCApp.cpp readme/PAPERS3_EDC.md
```

Expected: Matches still exist for the old screensaver tab and page.

**Step 2: Implement the minimal navigation change**

- Rename `AppPage::Screensaver` to `AppPage::Messages`
- Rename renderer entrypoint from `drawScreensaverPage()` to `drawMessagesPage()`
- Change bottom tab label from `屏保` to `消息`
- Update footer navigation so the third tab opens the message page
- Update EDC readme page list to remove screensaver and mention messages

**Step 3: Run the check again**

Run:
```bash
rg -n "Screensaver|屏保" examples/PaperS3EDC/PaperS3EDCModel.h examples/PaperS3EDC/PaperS3EDCRenderer.h examples/PaperS3EDC/PaperS3EDCRenderer.cpp examples/PaperS3EDC/PaperS3EDCApp.cpp readme/PAPERS3_EDC.md
```

Expected: No matches for the active UI path except historical plan docs.

**Step 4: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

**Step 5: Commit**

```bash
git add examples/PaperS3EDC/PaperS3EDCModel.h examples/PaperS3EDC/PaperS3EDCRenderer.h examples/PaperS3EDC/PaperS3EDCRenderer.cpp examples/PaperS3EDC/PaperS3EDCApp.cpp readme/PAPERS3_EDC.md
git commit -m "feat: replace screensaver tab with messages"
```

### Task 2: Add message data model and local app state

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCModel.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Write the failing check**

Run:
```bash
rg -n "MessageItem|MessagePage|MessageDetail|messageCount|selectedMessageIndex|messageScrollOffset|pollIntervalChargingSec|pollIntervalBatterySec" examples/PaperS3EDC/PaperS3EDCModel.h examples/PaperS3EDC/PaperS3EDCApp.cpp
```

Expected: No matches yet.

**Step 2: Add the minimal state model**

- Add `MessageItem` struct with `message_id`, `title`, `body`, `sent_at`, `source`, `read`
- Add message-related app state:
  - cached message array
  - message count
  - selected message index
  - selected message detail index
  - detail scroll offset
  - polling intervals for charging and battery
  - last message cursor
- Seed a few fake local messages so the UI can render before real network integration

**Step 3: Run the check again**

Run the same `rg` command.

Expected: New symbols exist.

**Step 4: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

**Step 5: Commit**

```bash
git add examples/PaperS3EDC/PaperS3EDCModel.h examples/PaperS3EDC/PaperS3EDCApp.cpp
git commit -m "feat: add papers3 edc message state"
```

### Task 3: Implement the message list and detail pages

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Write the failing check**

Run:
```bash
rg -n "drawMessagesPage|drawMessageDetailPage|message detail|消息详情|未读" examples/PaperS3EDC/PaperS3EDCRenderer.h examples/PaperS3EDC/PaperS3EDCRenderer.cpp examples/PaperS3EDC/PaperS3EDCApp.cpp
```

Expected: The detail page and list renderer do not exist yet.

**Step 2: Implement the minimal UI**

- Render the message list page in the old screensaver slot
- Show title, summary, time, and unread marker for each item
- Add message detail page
- Render long message body in a scrollable text viewport
- Keep navigation restrained and EPD-friendly

**Step 3: Add touch navigation**

- Tap a list row to enter detail
- Tap detail footer or top action to go back
- Add simple step-based scrolling for long detail text

**Step 4: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

**Step 5: Commit**

```bash
git add examples/PaperS3EDC/PaperS3EDCRenderer.h examples/PaperS3EDC/PaperS3EDCRenderer.cpp examples/PaperS3EDC/PaperS3EDCApp.cpp
git commit -m "feat: add papers3 edc message pages"
```

### Task 4: Add configurable polling settings

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCModel.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Write the failing check**

Run:
```bash
rg -n "2 秒|10 秒|poll interval|拉取间隔|插电拉取间隔|电池拉取间隔" examples/PaperS3EDC/PaperS3EDCModel.h examples/PaperS3EDC/PaperS3EDCApp.cpp examples/PaperS3EDC/PaperS3EDCRenderer.cpp
```

Expected: No configurable polling UI exists yet.

**Step 2: Implement the minimal settings support**

- Add default config values:
  - charging `2`
  - battery `10`
- Extend settings page with the two polling interval rows
- Add touch handling to adjust values in bounded steps
- Keep the settings text concise enough for the current layout

**Step 3: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

**Step 4: Verify on device**

Run:
```bash
pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101
```

Expected: Settings page shows editable charging and battery polling intervals.

**Step 5: Commit**

```bash
git add examples/PaperS3EDC/PaperS3EDCModel.h examples/PaperS3EDC/PaperS3EDCApp.cpp examples/PaperS3EDC/PaperS3EDCRenderer.cpp
git commit -m "feat: add papers3 edc message polling settings"
```

### Task 5: Extend storage for messages and polling configuration

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCStorage.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCStorage.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Write the failing check**

Run:
```bash
rg -n "message|cursor|pollIntervalChargingSec|pollIntervalBatterySec|selectedMessageIndex|messageScrollOffset" examples/PaperS3EDC/PaperS3EDCStorage.h examples/PaperS3EDC/PaperS3EDCStorage.cpp
```

Expected: Storage support for message state is incomplete or absent.

**Step 2: Implement the minimal persistence**

- Save and load cached messages
- Save and load message cursor
- Save and load polling intervals
- Preserve existing task/config persistence behavior

**Step 3: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

**Step 4: Hardware verification**

Run:
```bash
pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101
```

Expected: Message cache and intervals persist across reboot.

**Step 5: Commit**

```bash
git add examples/PaperS3EDC/PaperS3EDCStorage.h examples/PaperS3EDC/PaperS3EDCStorage.cpp examples/PaperS3EDC/PaperS3EDCApp.cpp
git commit -m "feat: persist papers3 edc messages and polling config"
```

### Task 6: Extend the sync boundary for message polling

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCSync.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCSync.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Write the failing check**

Run:
```bash
rg -n "pullMessages|message cursor|relay|messages\\?device_id" examples/PaperS3EDC/PaperS3EDCSync.h examples/PaperS3EDC/PaperS3EDCSync.cpp examples/PaperS3EDC/PaperS3EDCApp.cpp
```

Expected: Message polling sync path does not exist yet.

**Step 2: Implement the minimal sync path**

- Add a fake `pullMessages()` hook behind `PaperS3EDCSyncService`
- Accept device id and cursor-like state from app state
- Return fake relay-backed message payloads first
- Merge new messages into local cache and update cursor
- Keep the old task sync path intact

**Step 3: Implement timed polling**

- Track the active polling interval based on charging state
- Trigger message polling in the main loop when the interval elapses
- Avoid polling while a Wi‑Fi password page is active

**Step 4: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

**Step 5: Hardware verification**

Run:
```bash
pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101
```

Expected: Device refreshes the message page on the configured polling cadence.

**Step 6: Commit**

```bash
git add examples/PaperS3EDC/PaperS3EDCSync.h examples/PaperS3EDC/PaperS3EDCSync.cpp examples/PaperS3EDC/PaperS3EDCApp.cpp
git commit -m "feat: add papers3 edc message polling sync"
```

### Task 7: Update docs for the message-based MVP

**Files:**
- Modify: `readme/PAPERS3_EDC.md`
- Modify: `README.md`
- Optionally modify: `docs/plans/2026-03-20-papers3-edc-design.md`

**Step 1: Write the failing check**

Run:
```bash
rg -n "屏保|screensaver|message polling|钉钉|relay" readme/PAPERS3_EDC.md README.md
```

Expected: Docs still describe the old screensaver-focused behavior.

**Step 2: Update docs**

- Replace screensaver references in active EDC docs with message page references
- Document the relay-service architecture
- Document default polling intervals
- Document that the device is read-only

**Step 3: Build sanity check**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

**Step 4: Commit**

```bash
git add readme/PAPERS3_EDC.md README.md
git commit -m "docs: update papers3 edc message workflow"
```
