# PaperS3 Bin Font Demo Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a standalone M5Stack PaperS3 demo in `BinFontLib` that loads `霞鹜文楷32.bin`, renders Chinese text, and reports render timing on-screen and over serial.

**Architecture:** Add a new PaperS3-focused example that reuses `BinFontRuntime`, `M5FontPlatform`, and `M5FontRenderer`. Keep embedded-specific glue in the example folder, and limit library changes to cases where file path handling or renderer hooks are needed for the demo.

**Tech Stack:** Arduino, M5Unified, SD, BinFontLib runtime/renderer, optional PlatformIO for build/upload.

---

### Task 1: Inspect existing demo entrypoints

**Files:**
- Read: `examples/BinTestInteractive/BinTestInteractive.ino`
- Read: `examples/_shared/BinTestApp.cpp`
- Read: `src/BinTestApp.h`
- Read: `src/platform/M5FontPlatform.h`
- Read: `src/platform/M5FontRenderer.h`

**Step 1: Confirm current reusable pieces**
- Identify what can be reused directly for PaperS3 setup, font loading, and text rendering.

**Step 2: Confirm actual font asset path**
- Use `霞鹜文楷32.bin` at repo root as the source test asset.

**Step 3: Record integration constraints**
- Note whether the existing example assumes SD root, a fixed mount path, or a specific M5 display mode.

**Step 4: Commit**
```bash
git add docs/plans/2026-03-15-papers3-binfont-design.md docs/plans/2026-03-15-papers3-binfont-implementation.md
# No commit in this environment unless explicitly requested.
```

### Task 2: Add buildable PaperS3 example scaffold

**Files:**
- Create: `examples/PaperS3BinFontDemo/PaperS3BinFontDemo.ino`
- Create: `examples/PaperS3BinFontDemo/PaperS3DemoApp.cpp`
- Create: `examples/PaperS3BinFontDemo/PaperS3DemoApp.h`
- Create: `examples/PaperS3BinFontDemo/partitions.csv` (if the existing example needs one and reuse is appropriate)
- Modify: `README.md`

**Step 1: Write the failing build expectation**
- Define the new entrypoint API in `examples/PaperS3BinFontDemo/PaperS3DemoApp.h`:
```cpp
#pragma once
void PaperS3Demo_setup();
void PaperS3Demo_loop();
```
- Create `PaperS3BinFontDemo.ino` to call those functions.

**Step 2: Run a build or compile check to verify it fails for missing implementation**
Run: project-specific Arduino/PlatformIO build command once the build target exists.
Expected: fail with missing symbols for `PaperS3Demo_setup` / `PaperS3Demo_loop`.

**Step 3: Write minimal implementation**
- Add a minimal `PaperS3DemoApp.cpp` that initializes serial and M5.
- Render a simple hardcoded message to confirm the new example owns the screen.

**Step 4: Run build again**
- Confirm the scaffold now compiles.

### Task 3: Implement font file discovery and load diagnostics

**Files:**
- Modify: `examples/PaperS3BinFontDemo/PaperS3DemoApp.cpp`
- Optionally modify: `src/platform/M5FontPlatform.h` (only if file lookup support must expand beyond SD root)

**Step 1: Write the failing behavior target**
- Define the desired search order for the font file, for example:
```cpp
static const char* kFontCandidates[] = {
  "/霞鹜文楷32.bin",
  "/fonts/霞鹜文楷32.bin",
  "/font/霞鹜文楷32.bin",
};
```
- Expect startup logs to print each attempted path and end with a clear error if none exist.

**Step 2: Run on device or compile with logging stubs**
- Verify the current code does not yet produce these detailed diagnostics.

**Step 3: Write minimal implementation**
- Add storage init, candidate path scan, and runtime load logic.
- Print success logs including chosen path, char count, and font height.
- Show a visible startup error screen when loading fails.

**Step 4: Validate behavior**
- Build and, if possible, run on hardware to confirm success or controlled failure.

### Task 4: Add preview screen rendering

**Files:**
- Modify: `examples/PaperS3BinFontDemo/PaperS3DemoApp.cpp`
- Modify: `examples/PaperS3BinFontDemo/PaperS3DemoApp.h` (if page state types are declared there)

**Step 1: Write the failing behavior target**
- Define fixed preview strings, for example:
```cpp
static const char* kPreviewTitle = "BinFontLib PaperS3";
static const char* kPreviewText = "你好，霞鹜文楷。\n这是 PaperS3 真机验证页面。";
```
- Expect a dedicated preview page with heading, sample text, and footer hints.

**Step 2: Validate initial failure**
- Confirm the current demo only shows the scaffold message.

**Step 3: Write minimal implementation**
- Add a page enum and a preview render function.
- Render heading, sample text, and basic status lines.
- Refresh the e-ink display correctly after drawing.

**Step 4: Validate preview**
- Build and run on hardware to confirm visible Chinese rendering.

### Task 5: Add glyph check page

**Files:**
- Modify: `examples/PaperS3BinFontDemo/PaperS3DemoApp.cpp`

**Step 1: Write the failing behavior target**
- Define a small glyph set such as:
```cpp
static const char* kGlyphSample = "霞鹜文楷你好世界测试";
```
- Expect a page that renders these glyphs with enough spacing for visual inspection.

**Step 2: Validate initial failure**
- Confirm there is no glyph check page yet.

**Step 3: Write minimal implementation**
- Add a glyph page renderer.
- Include basic per-page label and any missing glyph diagnostics in serial output.

**Step 4: Validate glyph page**
- Confirm glyphs appear and are not obviously clipped.

### Task 6: Add render benchmark page

**Files:**
- Modify: `examples/PaperS3BinFontDemo/PaperS3DemoApp.cpp`
- Optionally create: `examples/PaperS3BinFontDemo/PaperS3Benchmark.h`
- Optionally create: `examples/PaperS3BinFontDemo/PaperS3Benchmark.cpp`

**Step 1: Write the failing behavior target**
- Define benchmark inputs and output shape:
```cpp
struct BenchmarkSummary {
  uint32_t iterations;
  uint32_t total_us;
  uint32_t max_us;
  uint32_t avg_us;
};
```
- Expect the benchmark page to run repeated renders of a fixed Chinese paragraph and print summary stats on screen and serial.

**Step 2: Validate initial failure**
- Confirm no benchmark summary exists yet.

**Step 3: Write minimal implementation**
- Time a fixed render loop with `micros()`.
- Compute total, average, and max.
- Render the summary and print a detailed serial log.

**Step 4: Validate benchmark**
- Build and run on hardware; confirm non-zero timings and readable output.

### Task 7: Add page switching and device-friendly interaction

**Files:**
- Modify: `examples/PaperS3BinFontDemo/PaperS3DemoApp.cpp`

**Step 1: Write the failing behavior target**
- Decide one interaction method based on PaperS3 capabilities already available in M5Unified, for example touch or timed auto-rotation.
- Expect the demo to navigate among preview, glyph, and benchmark pages.

**Step 2: Validate initial failure**
- Confirm the current app stays on one page.

**Step 3: Write minimal implementation**
- Add state transitions and a small footer hint.
- Keep the interaction simple and robust for e-ink refreshes.

**Step 4: Validate navigation**
- Confirm all pages are reachable on device.

### Task 8: Document build, flash, and font placement

**Files:**
- Modify: `README.md`
- Optionally create: `readme/PAPERS3_DEMO.md`

**Step 1: Document exact usage**
- Explain how to build/flash the PaperS3 demo.
- Explain where `霞鹜文楷32.bin` must live for SD or filesystem loading.
- Explain expected serial output and the connected device path `/dev/cu.usbmodem2101` observed during setup.

**Step 2: Verification checklist**
- Include a concise checklist for successful on-device validation.

**Step 3: Commit**
```bash
git add README.md readme/PAPERS3_DEMO.md examples/PaperS3BinFontDemo docs/plans
git status
# No commit in this environment unless explicitly requested.
```

### Task 9: Final verification

**Files:**
- Validate: `examples/PaperS3BinFontDemo/*`
- Validate: any modified `src/platform/*`

**Step 1: Run build verification**
- Use the project-appropriate build flow to compile the PaperS3 demo.

**Step 2: Run hardware verification**
- Flash to the connected device.
- Open serial output and capture the font load result and benchmark summary.

**Step 3: Record results**
- Save the exact commands used and the observed success/failure state in the final handoff.
