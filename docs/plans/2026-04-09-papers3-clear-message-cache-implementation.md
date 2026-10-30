# PaperS3 Clear Message Cache Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 在设置页增加“清空消息缓存”按钮，用于快速清空设备本地消息与游标状态。

**Architecture:** 不新增页面和确认流，直接在设置页触发一次本地状态重置。实现集中在设置页行数、点击处理和 `AppState` 消息字段清空三处，最后复用现有持久化与整页重绘路径。

**Tech Stack:** C++, M5Unified, LittleFS, PlatformIO

---

### Task 1: Add Clear-Message Action to Settings UI

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`

**Step 1: Increase settings row count**

Change the local constant from `7` to `8`.

**Step 2: Add the new row label**

Insert `清空消息缓存` before the current `返回今日` row in `drawSettingsPage(...)`.

**Step 3: Rebuild and inspect settings layout**

Run: `pio run -e papers3-edc`
Expected: `SUCCESS`

### Task 2: Implement Clear-Message State Reset

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Add a small helper that clears message state**

Reset:
- `messageCount = 0`
- `messages[] = {}`
- `messagePage.currentPageIndex = 0`
- `lastMessageCursor[0] = '\\0'`

**Step 2: Wire settings row handling**

Update the settings touch loop to iterate `8` rows and map:
- row `6` -> clear message cache
- row `7` -> return today

**Step 3: Persist and re-render**

After clearing:
- call `persistState()`
- set status text to `消息已清空`
- redraw settings page

**Step 4: Rebuild and check behavior**

Run: `pio run -e papers3-edc`
Expected: `SUCCESS`

### Task 3: Upload and Manual Verification

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Build final firmware**

Run: `pio run -e papers3-edc`
Expected: `SUCCESS`

**Step 2: Upload to device**

Run: `pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101`
Expected: `SUCCESS`

**Step 3: Manual verification**

Check on device:
- 设置页出现 `清空消息缓存`
- 点击后消息页变为空态
- 重启后消息仍为空
- Wi‑Fi 与刷新模式设置不丢失

**Step 4: Commit**

```bash
git add docs/plans/2026-04-09-papers3-clear-message-cache-design.md \
  docs/plans/2026-04-09-papers3-clear-message-cache-implementation.md \
  examples/PaperS3EDC/PaperS3EDCRenderer.cpp \
  examples/PaperS3EDC/PaperS3EDCApp.cpp
git commit -m "feat: add clear message cache action"
```
