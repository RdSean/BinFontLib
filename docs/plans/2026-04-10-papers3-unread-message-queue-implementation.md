# PaperS3 Unread Message Queue Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 把设备消息同步从时间游标模型切换为未读消息队列模型，设备默认只拉未读消息，后端返回后立即标记为已读。

**Architecture:** 后端 `messages` 表增加 `is_read` 状态，API 默认返回按时间正序排序的未读消息，并在成功响应前批量标记为已读。设备端移除 `since_ts` 依赖，只发送受缓存上限约束的 `limit`；`filter=all` 与 `debug=1` 作为不污染正常读取状态的排查路径保留。

**Tech Stack:** FastAPI, SQLAlchemy, SQLite, pytest, Arduino C++, PlatformIO

---

### Task 1: Add Read-State Storage to Backend Messages

**Files:**
- Modify: `backend/app/models.py`
- Modify: `backend/app/services/message_service.py`
- Test: `backend/tests/test_device_api.py`

**Step 1: Write failing backend tests for unread filtering**

Add tests that verify:
- new messages default to unread
- unread query returns oldest unread first
- all query returns newest first without changing read state

**Step 2: Run tests to verify they fail**

Run: `pytest backend/tests/test_device_api.py -q`
Expected: FAIL because `is_read` field and unread logic do not exist yet

**Step 3: Add `is_read` model field and query helpers**

Implement:
- `Message.is_read`
- helper to list unread oldest-first
- helper to list all newest-first
- helper to mark a batch as read

**Step 4: Run tests to verify partial backend model/query pass**

Run: `pytest backend/tests/test_device_api.py -q`
Expected: the new model/query assertions pass or move failure to API layer

### Task 2: Replace Device API Cursor Contract With Filter Contract

**Files:**
- Modify: `backend/app/api/device.py`
- Modify: `backend/app/schemas.py`
- Modify: `backend/app/services/message_service.py`
- Test: `backend/tests/test_device_api.py`
- Test: `backend/tests/test_end_to_end_cursor_flow.py`

**Step 1: Write failing API tests for new request semantics**

Cover:
- default request behaves as `filter=unread`
- `filter=all` returns newest `limit`
- `debug=1` does not mutate read state

**Step 2: Run tests to verify they fail**

Run: `pytest backend/tests/test_device_api.py backend/tests/test_end_to_end_cursor_flow.py -q`
Expected: FAIL because API still expects `since_ts`

**Step 3: Implement new API parameters**

Change API to accept:
- `device_id`
- `limit`
- `filter=unread|all`
- `debug=0|1`

Behavior:
- default unread path returns oldest unread and marks them read
- `all` path returns newest `limit` without mutating read state
- `debug=1` never mutates read state

**Step 4: Remove cursor-specific response assumptions**

Update schema/tests so device path no longer depends on `since_ts` semantics as the primary contract.

**Step 5: Run tests to verify backend API passes**

Run: `pytest backend/tests/test_device_api.py backend/tests/test_end_to_end_cursor_flow.py -q`
Expected: PASS

### Task 3: Update Device Sync Client to Unread Queue Pulling

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCSync.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCSync.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCModel.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCStorage.cpp`

**Step 1: Remove device dependency on message cursor**

Stop sending:
- `since_ts`
- cursor-jump logic
- backlog discard logic

Stop treating `lastMessageCursor` as sync state.

**Step 2: Clamp request size to device-safe capacity**

Define a safe limit such as:
- `kMessageRequestLimit = min(kMaxMessages, 16)`

Ensure the device never requests more than it can safely cache/display.

**Step 3: Implement unread/default pull**

Device default request should send only:
- `device_id`
- `limit`

Parse returned messages and merge into local cache as before.

**Step 4: Keep optional debug/all path for manual testing**

Allow future manual calls or dev-only toggles to request `filter=all&debug=1`, but do not make that the normal device path.

**Step 5: Run firmware build**

Run: `pio run -e papers3-edc`
Expected: `SUCCESS`

### Task 4: Simplify Clear Message Cache Behavior

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCStorage.cpp`

**Step 1: Remove cursor/backlog-specific clear logic**

Clear action should only reset:
- local messages
- local page index

Do not perform cursor jump or backlog-discard bookkeeping.

**Step 2: Persist cleaned local state**

Ensure save/load no longer depend on cursor state for device sync correctness.

**Step 3: Rebuild firmware**

Run: `pio run -e papers3-edc`
Expected: `SUCCESS`

### Task 5: End-to-End Verification and Upload

**Files:**
- Modify: `backend/app/api/device.py`
- Modify: `backend/app/services/message_service.py`
- Modify: `examples/PaperS3EDC/PaperS3EDCSync.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Run backend tests**

Run: `pytest backend/tests/test_device_api.py backend/tests/test_end_to_end_cursor_flow.py -q`
Expected: PASS

**Step 2: Build firmware**

Run: `pio run -e papers3-edc`
Expected: `SUCCESS`

**Step 3: Upload firmware**

Run: `pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101`
Expected: `SUCCESS`

**Step 4: Manual verification**

Verify:
- 钉钉新消息默认以未读进入数据库
- 设备默认请求只收到未读消息
- 收到后这些消息在后端被标记为已读
- 再次轮询不会重复收到同一批消息
- `filter=all` / `debug=1` 不污染已读状态
- 设备清空本地消息后，再来一条新消息能直接收到

**Step 5: Commit**

```bash
git add docs/plans/2026-04-10-papers3-unread-message-queue-design.md \
  docs/plans/2026-04-10-papers3-unread-message-queue-implementation.md \
  backend/app/models.py \
  backend/app/api/device.py \
  backend/app/services/message_service.py \
  backend/tests/test_device_api.py \
  backend/tests/test_end_to_end_cursor_flow.py \
  examples/PaperS3EDC/PaperS3EDCSync.cpp \
  examples/PaperS3EDC/PaperS3EDCSync.h \
  examples/PaperS3EDC/PaperS3EDCModel.h \
  examples/PaperS3EDC/PaperS3EDCStorage.cpp \
  examples/PaperS3EDC/PaperS3EDCApp.cpp
git commit -m "feat: switch device sync to unread message queue"
```
