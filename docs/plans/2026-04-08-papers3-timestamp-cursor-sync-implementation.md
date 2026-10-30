# PaperS3 Timestamp Cursor Sync Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace fragile numeric message cursors with ISO timestamp cursors and ensure API/worker always use the same configured database file.

**Architecture:** The backend device API will paginate messages by `sent_at` timestamp instead of message `id`, returning an ISO `next_cursor` string. The PaperS3 device will persist that string as its message cursor and omit the cursor on first pull when none is saved. Backend runtime config will use one authoritative `DATABASE_URL` source for both API and worker.

**Tech Stack:** FastAPI, SQLAlchemy, SQLite, Arduino, HTTPClient, ArduinoJson, existing PaperS3 storage/sync modules

---

### Task 1: Inspect current numeric cursor usage and database config defaults

**Files:**
- Modify: `backend/app/api/device.py`
- Modify: `backend/app/services/message_service.py`
- Modify: `backend/app/config.py`
- Modify: `examples/PaperS3EDC/PaperS3EDCSync.cpp`

**Step 1: Confirm current numeric cursor path**

Run:
```bash
rg -n "since_id|next_cursor|list_messages_after|parseCursorSinceId|writeCursor" backend/app examples/PaperS3EDC
```

Expected: backend and device still use numeric cursor behavior.

**Step 2: Confirm current database default behavior**

Run:
```bash
rg -n "DATABASE_URL|sqlite:///./data/app.db|database=%s" backend/app backend/run_stream_worker.py backend/dev.sh
```

Expected: config still includes a relative fallback and runtime logs show database path usage.

### Task 2: Add failing backend tests for timestamp cursor behavior

**Files:**
- Modify: `backend/tests/test_device_api.py`
- Modify: `backend/tests/test_end_to_end_cursor_flow.py`

**Step 1: Write the failing tests**

Add tests that verify:

- API accepts no cursor and returns messages plus ISO `next_cursor`
- API accepts `since_ts` and returns only newer messages
- API does not rely on numeric `since_id`

**Step 2: Run backend tests to verify RED**

Run:
```bash
cd backend && ./.venv/bin/pytest tests/test_device_api.py tests/test_end_to_end_cursor_flow.py -q
```

Expected: FAIL because API still expects numeric cursor semantics.

### Task 3: Implement backend timestamp cursor pagination

**Files:**
- Modify: `backend/app/api/device.py`
- Modify: `backend/app/services/message_service.py`
- Modify: `backend/app/schemas.py`

**Step 1: Add timestamp-based query support**

Implement service logic to:

- list messages after a given `sent_at` timestamp
- order by `sent_at`, then `id`
- apply the existing limit/has_more pattern

**Step 2: Update API request and response contract**

Change the device endpoint to:

- accept optional `since_ts`
- return string `next_cursor`

**Step 3: Run backend tests to verify GREEN**

Run:
```bash
cd backend && ./.venv/bin/pytest tests/test_device_api.py tests/test_end_to_end_cursor_flow.py -q
```

Expected: PASS.

### Task 4: Remove ambiguous database fallback and verify one configured database path

**Files:**
- Modify: `backend/app/config.py`
- Modify: `backend/dev.sh`
- Modify: `backend/run_stream_worker.py`

**Step 1: Make `DATABASE_URL` the single authoritative database source**

Ensure:

- config does not silently fall back to a different runtime-relative database for normal development
- API and worker logs make the configured database path obvious

**Step 2: Verify API and worker share the same configured path**

Run:
```bash
cd backend && ./dev.sh restart
cd backend && ./dev.sh status
```

Expected: `api` and `worker` are running, and logs indicate the same configured database target.

### Task 5: Add failing device-side checks for ISO cursor handling

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCSync.cpp`

**Step 1: Write the failing inspection checks**

Run:
```bash
rg -n "since_id|parseCursorSinceId|writeCursor" examples/PaperS3EDC/PaperS3EDCSync.cpp
```

Expected: device still uses numeric cursor parsing and request building.

### Task 6: Implement device ISO cursor persistence and first-pull behavior

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCSync.cpp`
- Modify: `examples/PaperS3EDC/PaperS3EDCStorage.cpp` (only if persistence format needs adjustment)

**Step 1: Replace numeric cursor helpers**

Implement helpers to:

- detect whether a saved cursor exists
- send `since_ts` only when the cursor string is non-empty
- copy returned ISO cursor strings directly into `lastMessageCursor`

**Step 2: Preserve no-cursor initial pull**

If `lastMessageCursor` is empty:

- omit `since_ts`
- accept backend `next_cursor`

**Step 3: Build firmware**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

### Task 7: Verify end-to-end with real DingTalk delivery

**Files:**
- No code changes required if previous steps pass

**Step 1: Start backend**

Run:
```bash
cd backend && ./dev.sh restart
```

Expected: `api` and `worker` are running.

**Step 2: Flash firmware**

Run:
```bash
pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101
```

Expected: upload succeeds.

**Step 3: Verify real message flow**

Expected:

- new DingTalk messages are saved into the configured database
- device API returns them for an older timestamp cursor
- a device with no cursor can fetch messages
- after a successful pull, the device stores and reuses the ISO timestamp cursor
