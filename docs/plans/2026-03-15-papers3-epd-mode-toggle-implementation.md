# PaperS3 EPD Mode Toggle Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add a dedicated runtime EPD refresh-mode toggle button to the PaperS3 demo so the user can compare `fast`, `text`, `quality`, and `fastest` on real hardware.

**Architecture:** Extend `PaperS3DemoApp.cpp` with a second header control, add a small enum plus helpers for EPD mode mapping and labels, and thread that selection through normal page refreshes and benchmark logs.

**Tech Stack:** Arduino, M5Unified, M5GFX, BinFontLib runtime/renderer, PlatformIO.

---

### Task 1: Add EPD mode state and labels

**Files:**
- Modify: `examples/PaperS3BinFontDemo/PaperS3DemoApp.cpp`

**Step 1: Define desired behavior**
- Add a runtime enum with four values: `Fast`, `Text`, `Quality`, `Fastest`
- Add helpers for short labels and `epd_mode_t` mapping

**Step 2: Validate current limitation**
Run: existing demo on device
Expected: only one header button exists; no EPD mode state is visible or changeable

**Step 3: Write minimal implementation**
- Add enum and global state
- Default to `Text` or the current behavior-preserving value

**Step 4: Verify by build**
Run: `pio run -e papers3-demo`
Expected: build succeeds with the new state plumbing

### Task 2: Add second header button and touch handling

**Files:**
- Modify: `examples/PaperS3BinFontDemo/PaperS3DemoApp.cpp`

**Step 1: Define desired behavior**
- Header shows `Bench:*` and `EPD:*` buttons side by side
- Touching the EPD button cycles through four modes

**Step 2: Validate initial failure**
Run: current build on device
Expected: tapping the future EPD button area does nothing

**Step 3: Write minimal implementation**
- Add button layout helpers
- Add `toggleEpdMode()` and touch hit-testing

**Step 4: Verify by build**
Run: `pio run -e papers3-demo`
Expected: build succeeds and the device responds to touches on both header buttons

### Task 3: Apply selected EPD mode to page refreshes and benchmark logs

**Files:**
- Modify: `examples/PaperS3BinFontDemo/PaperS3DemoApp.cpp`

**Step 1: Define desired behavior**
- `Preview`, `Glyphs`, and benchmark summary pages refresh using the selected EPD mode
- Benchmark logs include both strategy and EPD mode labels

**Step 2: Validate initial failure**
Run: current build on device
Expected: logs only show benchmark strategy; refresh mode cannot be distinguished from logs

**Step 3: Write minimal implementation**
- Route `display()` calls through the current mode helper
- Add `epd=` to benchmark log lines and status text

**Step 4: Verify by build and device capture**
Run: `pio run -e papers3-demo`
Run: `pio run -e papers3-demo -t upload --upload-port /dev/cu.usbmodem2101`
Expected: build/upload succeed and serial logs report the active EPD mode

### Task 4: Final device verification

**Files:**
- Validate: `examples/PaperS3BinFontDemo/PaperS3DemoApp.cpp`
- Validate: `docs/plans/2026-03-15-papers3-epd-mode-toggle-design.md`
- Validate: `docs/plans/2026-03-15-papers3-epd-mode-toggle-implementation.md`

**Step 1: Boot and capture logs**
Run: serial capture on `/dev/cu.usbmodem2101`
Expected: startup logs show font load success and benchmark output with `mode=` and `epd=` labels

**Step 2: Manual interaction check**
- Tap the new EPD header button repeatedly
- Confirm header label cycles and pages re-render

**Step 3: Report results**
- Summarize exact build/upload commands
- Record observed benchmark mode and EPD mode from serial output
