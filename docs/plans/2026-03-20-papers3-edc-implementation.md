# PaperS3 EDC MVP Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build the first usable MVP of the PaperS3 child-focused EDC task device, including a local-first daily task UI, completion flow, screensaver, device settings, and a sync-ready data layer.

**Architecture:** Keep the first delivery device-first and local-first. Reuse the existing PaperS3 demo stack in `BinFontLib`, but split the new EDC app into focused modules for UI state, local task storage, device configuration, sync boundaries, render services, and font management. Keep render logic and font management isolated from business rules so future e-ink refresh tuning, typography changes, and draw-pipeline optimization can happen in one place without touching task, points, sync, or settings logic. Defer cloud integration until the local PaperS3 experience is visually correct and interaction-safe on e-ink.

**Tech Stack:** Arduino, M5Unified, M5GFX, LittleFS, BinFontLib, PlatformIO, PaperS3 touch input, lightweight JSON serialization for local task/config data.

---

### Task 1: Create the new PaperS3 EDC example scaffold

**Files:**
- Create: `examples/PaperS3EDC/PaperS3EDC.cpp`
- Create: `examples/PaperS3EDC/PaperS3EDCApp.h`
- Create: `examples/PaperS3EDC/PaperS3EDCApp.cpp`
- Modify: `platformio.ini`
- Modify: `readme/PAPERS3_DEMO.md`

**Step 1: Define the failing target**
- Add a new PlatformIO environment for the EDC app, for example `papers3-edc`.
- Expect the build to fail before the new example source files exist.

**Step 2: Run build to verify it fails**
Run: `pio run -e papers3-edc`
Expected: FAIL with missing entry source or undefined setup/loop symbols.

**Step 3: Write minimal implementation**
- Add a new example entrypoint that calls `PaperS3EDC_setup()` and `PaperS3EDC_loop()`.
- Add a minimal app scaffold that initializes `M5`, serial output, and the display.
- Render a plain placeholder title so the new app owns the screen.

**Step 4: Run build to verify it passes**
Run: `pio run -e papers3-edc`
Expected: PASS and produce a firmware binary.

**Step 5: Commit**
```bash
git add examples/PaperS3EDC platformio.ini readme/PAPERS3_DEMO.md
git commit -m "feat: add papers3 edc app scaffold"
```

### Task 2: Define local app state and fake task data

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Define the failing target**
- Introduce local-first data structures for `TaskItem`, `DailySummary`, `PointsSummary`, `BatteryState`, and `AppPage`.
- Expect the app to show fake in-memory tasks before any persistence exists.

**Step 2: Run build to verify the new symbols are still missing**
Run: `pio run -e papers3-edc`
Expected: FAIL if references are added before implementations are complete.

**Step 3: Write minimal implementation**
- Add small structs for tasks and UI summaries.
- Seed 4–6 fake Chinese tasks directly in code.
- Add a single source of truth for current page, selected task index, and points total.

**Step 4: Run build to verify it passes**
Run: `pio run -e papers3-edc`
Expected: PASS.

**Step 5: Commit**
```bash
git add examples/PaperS3EDC/PaperS3EDCApp.h examples/PaperS3EDC/PaperS3EDCApp.cpp
git commit -m "feat: add papers3 edc local app state"
```

### Task 3: Build the visual system for e-ink UI

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`
- Optionally create: `examples/PaperS3EDC/PaperS3EDCTheme.h`

**Step 1: Define the failing target**
- Add reusable layout constants and drawing helpers for the product style: top status bar, thin-line cards, rectangular task markers, footer hint text, and page title areas.
- Expect the placeholder screen to be replaced by the approved “student daybook” visual language.

**Step 2: Run build or device check to verify the old placeholder still shows**
Run: `pio run -e papers3-edc`
Expected: PASS build, but device UI still lacks the intended style before implementation.

**Step 3: Write minimal implementation**
- Add layout constants for padding, status bar height, row height, and section spacing.
- Add helpers such as `drawStatusBar()`, `drawRectButton()`, `drawTaskMarker()`, and `displayFrame()`.
- Reuse the bin font renderer where it improves Chinese title/body text presentation.

**Step 4: Run build to verify it passes**
Run: `pio run -e papers3-edc`
Expected: PASS.

**Step 5: Commit**
```bash
git add examples/PaperS3EDC/PaperS3EDCApp.cpp examples/PaperS3EDC/PaperS3EDCTheme.h
git commit -m "feat: add papers3 edc eink visual system"
```

### Task 4: Implement the main Today screen

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Define the failing target**
- The main screen must show battery, total task count, completed count, date, and the ordered task list.
- Task rows must use rectangular completion markers, not circular checkboxes.
- If the full list cannot fit, the lower area should degrade gracefully into summary information.

**Step 2: Run device build and observe failure of missing UI**
Run: `pio run -e papers3-edc`
Expected: PASS build, but the screen still lacks the intended Today layout before implementation.

**Step 3: Write minimal implementation**
- Add `renderTodayPage()`.
- Render header date and task summary.
- Render visible task rows with selection highlight.
- Add graceful fallback for long lists: show count summary instead of squeezing rows too tightly.

**Step 4: Run build and hardware verification**
Run: `pio run -e papers3-edc`
Run: `pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem2101`
Expected: PASS, and the Today screen appears on device.

**Step 5: Commit**
```bash
git add examples/PaperS3EDC/PaperS3EDCApp.cpp
git commit -m "feat: add papers3 edc today screen"
```

### Task 5: Implement selection and confirmation flow

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Define the failing target**
- Tapping a task row or its left rectangular marker should move the app into a confirmation state.
- Confirmation should mark the task complete, increment points, and update completion counters immediately.

**Step 2: Run device build and verify the interaction is missing**
Run: `pio run -e papers3-edc`
Expected: PASS build, but no task completion interaction exists yet.

**Step 3: Write minimal implementation**
- Add task hit-testing for row taps and marker taps.
- Add a dedicated confirmation page or modal-like full-page confirmation layout.
- Update task status, points, and completed count on confirmation.
- Return to the Today page after confirmation.

**Step 4: Run hardware verification**
Run: `pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem2101`
Expected: Device supports select → confirm → complete flow.

**Step 5: Commit**
```bash
git add examples/PaperS3EDC/PaperS3EDCApp.cpp
git commit -m "feat: add papers3 edc completion flow"
```

### Task 6: Implement the points and contribution summary page

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Define the failing target**
- Add a secondary page that shows total points and a small rectangular contribution heatmap summary.
- The view should remain visually restrained and e-ink-friendly.

**Step 2: Run device build and verify the page does not exist**
Run: `pio run -e papers3-edc`
Expected: PASS build, but no points/heatmap view exists yet.

**Step 3: Write minimal implementation**
- Add `renderProgressPage()`.
- Render total points, streak placeholder, and a small 2–4 row rectangular contribution grid.
- Use fake local data first.

**Step 4: Run build and hardware verification**
Run: `pio run -e papers3-edc`
Run: `pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem2101`
Expected: PASS, and the summary page is viewable on device.

**Step 5: Commit**
```bash
git add examples/PaperS3EDC/PaperS3EDCApp.cpp
git commit -m "feat: add papers3 edc progress page"
```

### Task 7: Implement the screensaver page

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`
- Optionally create: `examples/PaperS3EDC/PaperS3EDCIllustration.h`

**Step 1: Define the failing target**
- Add a screensaver-style page that shows the current date and either task excerpts or task summary counts.
- Include restrained Notion-style line illustration elements without overwhelming the text.

**Step 2: Run device build and verify the page is missing**
Run: `pio run -e papers3-edc`
Expected: PASS build, but there is no dedicated screensaver page yet.

**Step 3: Write minimal implementation**
- Add `renderScreensaverPage()`.
- Use the approved fallback rule: if the task list is too long, switch to count summary.
- Add a tiny line-art helper set that uses thin black lines only.

**Step 4: Run hardware verification**
Run: `pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem2101`
Expected: Device can display the screensaver page with elegant date/task composition.

**Step 5: Commit**
```bash
git add examples/PaperS3EDC/PaperS3EDCApp.cpp examples/PaperS3EDC/PaperS3EDCIllustration.h
git commit -m "feat: add papers3 edc screensaver"
```

### Task 8: Implement the device settings and Wi‑Fi pages

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`
- Optionally create: `examples/PaperS3EDC/PaperS3EDCKeyboard.h`
- Optionally create: `examples/PaperS3EDC/PaperS3EDCKeyboard.cpp`

**Step 1: Define the failing target**
- Add a settings entry from the main UI.
- Implement Wi‑Fi scan, SSID selection, and password entry using a rectangular on-device keyboard.

**Step 2: Run device build and verify the settings flow is missing**
Run: `pio run -e papers3-edc`
Expected: PASS build, but no settings navigation or Wi‑Fi flow exists yet.

**Step 3: Write minimal implementation**
- Add `renderSettingsPage()`.
- Add `renderWifiListPage()` and `renderWifiPasswordPage()`.
- Add a minimal on-device keyboard grid with letters, numbers, backspace, case toggle, and confirm.
- Save the selected SSID and password to config storage after successful input.

**Step 4: Run hardware verification**
Run: `pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem2101`
Expected: Device can scan Wi‑Fi, accept password input, and persist config.

**Step 5: Commit**
```bash
git add examples/PaperS3EDC/PaperS3EDCApp.cpp examples/PaperS3EDC/PaperS3EDCKeyboard.h examples/PaperS3EDC/PaperS3EDCKeyboard.cpp
git commit -m "feat: add papers3 edc settings and wifi flow"
```

### Task 9: Add local persistence for tasks, points, and config

**Files:**
- Create: `examples/PaperS3EDC/PaperS3EDCStorage.h`
- Create: `examples/PaperS3EDC/PaperS3EDCStorage.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Define the failing target**
- Persist task list, completion state, point totals, pending sync events, and Wi‑Fi/device config in local storage.
- The device should recover state after reboot.

**Step 2: Run build and verify persistence is absent**
Run: `pio run -e papers3-edc`
Expected: PASS build, but state is lost after reboot before implementation.

**Step 3: Write minimal implementation**
- Add a small storage layer over `LittleFS`.
- Store JSON files for app state, device config, and pending sync queue.
- Load persisted state at startup and save after meaningful changes.

**Step 4: Run build and hardware verification**
Run: `pio run -e papers3-edc`
Run: `pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem2101`
Expected: Device preserves state across reboot.

**Step 5: Commit**
```bash
git add examples/PaperS3EDC/PaperS3EDCStorage.h examples/PaperS3EDC/PaperS3EDCStorage.cpp examples/PaperS3EDC/PaperS3EDCApp.cpp
git commit -m "feat: add papers3 edc local persistence"
```

### Task 10: Add a sync boundary with fake transport hooks

**Files:**
- Create: `examples/PaperS3EDC/PaperS3EDCSync.h`
- Create: `examples/PaperS3EDC/PaperS3EDCSync.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Define the failing target**
- Prepare the device app for future cloud integration without binding it to final APIs yet.
- Add fake sync hooks that consume local pending events and log payload shapes over serial.

**Step 2: Run build and verify sync boundary does not exist**
Run: `pio run -e papers3-edc`
Expected: PASS build, but no sync abstraction exists before implementation.

**Step 3: Write minimal implementation**
- Add a sync interface for `pullTodayTasks()` and `pushPendingEvents()`.
- Initially implement it as a fake serial logger plus no-op responses.
- Trigger manual sync from the settings page.

**Step 4: Run build and hardware verification**
Run: `pio run -e papers3-edc`
Run: `pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem2101`
Expected: Device can perform a fake sync and log event payloads.

**Step 5: Commit**
```bash
git add examples/PaperS3EDC/PaperS3EDCSync.h examples/PaperS3EDC/PaperS3EDCSync.cpp examples/PaperS3EDC/PaperS3EDCApp.cpp
git commit -m "feat: add papers3 edc sync boundary"
```

### Task 11: Document the PaperS3 EDC MVP flow

**Files:**
- Modify: `README.md`
- Modify: `readme/PAPERS3_DEMO.md`
- Optionally create: `readme/PAPERS3_EDC.md`

**Step 1: Define documentation target**
- Document how to build, flash, and operate the new `papers3-edc` target.
- Document page flow, local-first behavior, and known limitations.

**Step 2: Verify docs are incomplete**
- Confirm current docs only cover the font demo and not the EDC app.

**Step 3: Write minimal implementation**
- Add build instructions, flashing steps, storage notes, and UI navigation summary.
- Include the current USB port example `/dev/cu.usbmodem2101` used locally.

**Step 4: Verify docs are saved**
Run: `git diff -- README.md readme/PAPERS3_DEMO.md readme/PAPERS3_EDC.md`
Expected: New EDC instructions are present.

**Step 5: Commit**
```bash
git add README.md readme/PAPERS3_DEMO.md readme/PAPERS3_EDC.md
git commit -m "docs: add papers3 edc usage guide"
```

### Task 12: Final verification

**Files:**
- Validate: `examples/PaperS3EDC/*`
- Validate: `platformio.ini`
- Validate: `readme/PAPERS3_DEMO.md`

**Step 1: Run build verification**
Run: `pio run -e papers3-edc`
Expected: PASS.

**Step 2: Run hardware verification**
Run: `pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem2101`
Run: serial capture for boot and manual interactions
Expected: Device boots, renders Today page, supports completion flow, and preserves state.

**Step 3: Verify the requirements checklist**
- Main task-first screen exists
- Screensaver exists with graceful fallback
- Completion flow exists and is immediate
- Points and contribution summary exist
- Settings and Wi‑Fi input exist
- Local persistence exists
- Sync boundary exists

**Step 4: Record final status**
- Summarize exact commands used
- List remaining gaps for cloud APIs and mini program implementation

