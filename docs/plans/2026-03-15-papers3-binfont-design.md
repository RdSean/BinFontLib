# PaperS3 Bin Font Demo Design

**Date:** 2026-03-15
**Repo:** `BinFontLib`
**Branch:** `papers3-binfont-demo`

## Goal

Create a standalone M5Stack PaperS3 demo inside `BinFontLib` that loads the real `霞鹜文楷32.bin` font file, renders Chinese text on device, and reports render timing over serial and on-screen. The demo is for verifying that the bin-font rendering path works correctly on PaperS3 hardware and is suitable for later migration into a closed-source PaperS3 project.

## Scope

### In scope
- Add a dedicated PaperS3 demo entrypoint in this repository
- Load the real root-level font file `霞鹜文楷32.bin`
- Render fixed Chinese sample text on the PaperS3 display
- Provide a simple on-device screen flow for preview, glyph checking, and timing
- Print clear diagnostics for font loading, missing files, and render timing
- Support flashing and serial inspection from a connected USB device

### Out of scope
- No `vlw` comparison
- No integration with `m5stack_game`
- No redesign of the bin font file format
- No generalized UI framework beyond what the demo needs

## Constraints
- The hardware target is M5Stack PaperS3 connected over USB (`/dev/cu.usbmodem2101` observed locally)
- The demo should stay isolated from the existing interactive example as much as practical
- The code should remain suitable for Arduino/PlatformIO-style embedded builds
- The font path should be simple and explicit to reduce debug time on hardware

## Recommended Approach

Create a new dedicated demo under `examples/PaperS3BinFontDemo/` and reuse the existing library runtime where possible. Keep the demo small, but structure it into a few focused modules so that rendering, benchmarking, and device glue stay easy to reason about.

This is preferred over patching the current example in place because it preserves the current demo and gives PaperS3-specific validation a clean home.

## Architecture

### 1. Demo entry
A new example entrypoint initializes M5PaperS3, serial logging, and storage, then hands off to a small application controller.

### 2. Font loading
The app loads `霞鹜文楷32.bin` from a known path. The first path checked should match the root-level file when packaged for the demo. If needed, fallback search paths can be added for SD/LittleFS deployment.

### 3. Rendering
The existing bin font renderer remains the core drawing path. The demo wraps it with PaperS3-friendly screen rendering that draws fixed test strings and a few single glyph checks.

### 4. Benchmarking
A focused benchmark runner repeatedly draws the same text, measures elapsed time, and reports summary statistics:
- iteration count
- total time
- average time
- slowest time
- optional per-character average

### 5. UI flow
The demo should expose a minimal page-based flow:
- `Preview`: fixed sample paragraphs and headings
- `Glyph Check`: selected Chinese characters one by one or in a short row
- `Benchmark`: repeat render test with visible and serial output

## Data Flow
1. Device boots and initializes display + serial
2. Storage mounts
3. Font file lookup runs and logs chosen path
4. Font parser/runtime loads metadata and glyph access tables
5. Preview screen renders sample Chinese text
6. Glyph screen renders selected glyphs for visual sanity check
7. Benchmark screen executes repeated renders and prints timing summary

## Error Handling
- Font file missing: show exact searched path on screen and serial
- Invalid font header: stop benchmark, print parser error
- Missing glyph: draw placeholder and log codepoint
- Storage mount failure: stay on startup error screen
- Render failure: print context including active page and text label

## Validation Strategy

### Build validation
- Build the new PaperS3 demo target locally
- Ensure no regressions to library headers used by the demo

### Hardware validation
- Flash to the connected PaperS3
- Open serial monitor on `/dev/cu.usbmodem2101`
- Verify startup logs, font load success, and benchmark output
- Confirm visible Chinese rendering on e-ink screen without obvious clipping

### Acceptance criteria
- Demo builds successfully
- Device boots into the new demo
- `霞鹜文楷32.bin` loads successfully on device
- Chinese text renders visibly and correctly enough for manual inspection
- Benchmark statistics print on screen and serial
- Failure states are understandable without reading code

## Files Expected
- New demo files under `examples/PaperS3BinFontDemo/`
- New documentation under `docs/plans/`
- Possibly small library changes if the current runtime needs PaperS3-specific glue or file-loading improvements

## Notes
- Per repo-level instructions in the current environment, this design document is saved but not committed automatically.
