# PaperS3 Multi-Font Diagnostics Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Extend the existing single-font settings diagnostics so multiple candidate bin fonts can be uploaded to LittleFS, loaded independently, and rendered on-device for validation.

**Architecture:** Keep the current main UI font unchanged. Add small independent preview font slots in the font manager for each candidate diagnostic font, then render a compact diagnostics panel in settings using those independent preview slots.

**Tech Stack:** Arduino, LittleFS, BinFontLib runtime/renderer, existing PaperS3 settings renderer, uploaded `.bin` font assets.

---

### Task 1: Add the new font files to device assets

**Files:**
- Copy: `data/LXGWWenKaiLite-for-edc-24.bin`
- Copy: `data/LXGWWenKai-24.bin`

**Step 1: Verify current asset state**

Run:
```bash
ls -1 data | rg '霞鹜文楷28\.bin|LXGWWenKaiLite-for-edc-24\.bin|LXGWWenKai-24\.bin'
```

Expected: Only the currently copied diagnostics fonts appear.

**Step 2: Copy missing assets**

Copy:

- `/Users/seanlee/开发文档/GithubCode/BinFontLib/LXGWWenKaiLite-for-edc-24.bin`
- `/Users/seanlee/开发文档/GithubCode/BinFontLib/LXGWWenKai-24.bin`

into:

- `data/`

### Task 2: Generalize preview font loading in the font manager

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCFontManager.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCFontManager.cpp`

**Step 1: Write the failing check**

Run:
```bash
rg -n "previewLoadedPath|drawPreviewText|tryLoadPreview" examples/PaperS3EDC/PaperS3EDCFontManager.*
```

Expected: Only the single preview-font path exists.

**Step 2: Implement minimal multi-font preview support**

- replace the single preview slot with a small fixed array of preview slots
- each slot tracks:
  - label
  - load state
  - path
  - font height
  - renderer/runtime objects
- expose small accessors for diagnostics rendering

**Step 3: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

### Task 3: Replace single-font diagnostics UI with compact multi-font diagnostics

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Write the failing check**

Run:
```bash
rg -n "28号字体诊断|previewLoadedPath|drawPreviewText" examples/PaperS3EDC/PaperS3EDCRenderer.cpp
```

Expected: Current UI still only handles the single `28.bin` diagnostic.

**Step 2: Implement compact multi-font diagnostics**

- replace the single `28号字体诊断` block with a `字体诊断` block
- render one compact subsection per preview font
- show:
  - label
  - load result
  - font height
  - short rendered sample if loaded

**Step 3: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

### Task 4: Update docs and flash assets plus firmware

**Files:**
- Modify: `readme/PAPERS3_EDC.md`

**Step 1: Update docs**

- mention multiple font diagnostics on the settings page
- mention the specific candidate fonts used for diagnostics

**Step 2: Upload LittleFS**

Run:
```bash
pio run -e papers3-edc -t uploadfs --upload-port /dev/cu.usbmodem1101
```

Expected: LittleFS image contains all diagnostic fonts and upload succeeds.

**Step 3: Upload firmware**

Run:
```bash
pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101
```

Expected: Firmware upload succeeds.

**Step 4: Verify on device**

Expected:

- settings page shows multiple font diagnostics
- each font clearly reports load success or failure
- loaded fonts render their Chinese sample visibly
