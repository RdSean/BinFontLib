# PaperS3 Message Fast Canvas Partial Refresh Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 在真实消息页回接 `Fast + Canvas` 局部刷新，只对最后一页新增消息的最小文本区域做局刷，并把页码变化限定为状态栏单独刷新。

**Architecture:** 保持现有整屏全刷作为稳定后备路径，只在消息页的“最后一页 + 新消息追加”场景启用 `Fast + Canvas`。渲染层负责计算新增消息实际覆盖的最小矩形，并在提交局刷前继续使用现有的 panel 坐标补偿；控制层只在满足条件时走这条专用路径，其余消息翻页和非最后页消息更新仍回退到稳定路径。

**Tech Stack:** C++, M5Unified, M5GFX `Panel_EPD`, `M5Canvas`, PlatformIO

---

### Task 1: Add Message Append Dirty-Rect Measurement

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Add renderer-side append refresh data structures**

Add compact structs that describe:
- previous/current last-page metrics
- previous/current message count
- append dirty rect in viewport coordinates
- whether append refresh is eligible

**Step 2: Implement last-page append rect measurement**

Add a helper that:
- compares the previous and current app state
- only enables append refresh when both states are on `AppPage::Messages`
- requires current page to be the last page after merge
- requires old page count and new page count to match, so only “fill current last page” is handled
- computes the top of the first newly appended display item on the last page
- returns the minimal viewport rect from that top to the bottom of the last new message block

**Step 3: Add focused render helpers**

Add helpers for:
- drawing the status bar only
- drawing the message viewport canvas clipped to a supplied dirty rect
- submitting a viewport sub-rect with `displayRegion(...)`

**Step 4: Build-check the renderer changes**

Run: `pio run -e papers3-edc`
Expected: `SUCCESS`

### Task 2: Route Real Message Page Refreshes Through Fast + Canvas

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Add a message-page partial refresh entry point**

Expose a renderer method that accepts:
- previous state snapshot
- current state snapshot
- requested refresh mode

Use this method to decide between:
- `status bar only` region submit
- `last-page append dirty rect` submit
- fallback to existing full-frame path

**Step 2: Force partial message submits onto `Fast`**

For `StatusOnly` and successful `MessageAppend` region submits:
- use `EpdRefreshMode::Fast`
- keep current `toPanelRegion(...)` compensation before `display(region)`

For non-eligible cases:
- keep existing mode-dependent full-frame behavior

**Step 3: Preserve full-page redraw fallback**

If append rect measurement fails, canvas creation fails, or the append crosses into a new page:
- redraw the whole message viewport and use existing full-frame submit path

**Step 4: Build-check the renderer routing**

Run: `pio run -e papers3-edc`
Expected: `SUCCESS`

### Task 3: Feed Previous State Into Message Refresh Decisions

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Snapshot state before message pulls and page flips**

Before:
- `maybePollMessages()`
- `runManualSync()` when messages change
- message-page tap pagination

capture the previous `AppState` needed for refresh decisions.

**Step 2: Use append path only for live message arrival**

When `pullMessages()` returns new data:
- jump to the last page
- call the new renderer message partial refresh method with previous/current state
- persist state after rendering

**Step 3: Restrict page navigation to status-bar-only region refresh**

When the user flips pages in the message page:
- keep the current page-content refresh stable
- submit the page-number/status change as a status-bar-only fast refresh

**Step 4: Build-check app integration**

Run: `pio run -e papers3-edc`
Expected: `SUCCESS`

### Task 4: Verify on Device and Upload

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Compile final firmware**

Run: `pio run -e papers3-edc`
Expected: `SUCCESS`

**Step 2: Upload to the connected device**

Run: `pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101`
Expected: `SUCCESS`

**Step 3: Manual device checks**

Verify on hardware:
- 新消息追加到最后一页时，只刷新新增文本覆盖到的矩形
- 顶部页码变化时只刷新状态栏
- 新消息追加跨页时回退到稳定刷新路径，不出现马赛克或缺字
- `Text` 模式下整屏刷新行为保持稳定，不被当作消息局刷主路径

**Step 4: Commit**

```bash
git add docs/plans/2026-04-09-papers3-message-fast-canvas-partial-refresh-implementation.md \
  examples/PaperS3EDC/PaperS3EDCRenderer.h \
  examples/PaperS3EDC/PaperS3EDCRenderer.cpp \
  examples/PaperS3EDC/PaperS3EDCApp.cpp
git commit -m "feat: add fast canvas message partial refresh"
```
