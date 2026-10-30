# PaperS3 Real Message Sync Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace the fake in-firmware message relay with a real HTTP pull from the local FastAPI backend, while keeping the rest of the PaperS3 app behavior unchanged.

**Architecture:** `PaperS3EDCSyncService::pullMessages()` becomes the integration boundary for real backend reads. It will build a GET request to the FastAPI device endpoint, parse the JSON response, merge messages into the existing local cache, and persist the returned cursor so the renderer and polling loop remain unchanged.

**Tech Stack:** Arduino, WiFi, HTTPClient, ArduinoJson, existing PaperS3 sync/storage/renderer modules, FastAPI backend response contract.

---

### Task 1: Add the failing integration checks and inspect current fake path

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCSync.cpp`

**Step 1: Write the failing checks**

Run:
```bash
rg -n "kRelayMessages|relay-cursor|GET /messages\\?device_id" examples/PaperS3EDC/PaperS3EDCSync.cpp
```

Expected: The fake relay seed path and fake GET log are still present.

**Step 2: Write the failing dependency check**

Run:
```bash
rg -n "HTTPClient|ArduinoJson|deserializeJson" examples/PaperS3EDC/PaperS3EDCSync.cpp
```

Expected: No real HTTP or JSON parsing exists yet.

### Task 2: Add real message fetch helpers

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCSync.cpp`

**Step 1: Implement request constants and helpers**

Add minimal constants for:

- backend base URL `http://192.168.3.181:8000`
- device endpoint `/api/device/messages`
- device token `paper-s3-local-token`
- request limit `8`

Add small helpers to:

- parse `lastMessageCursor` into integer `since_id`
- stringify the returned `next_cursor`
- copy backend fields into `MessageItem`

**Step 2: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

### Task 3: Replace fake relay seeds with HTTP GET

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCSync.cpp`

**Step 1: Implement the real request**

In `pullMessages()`:

- keep the existing Wi-Fi connectivity guard
- create the request URL with `device_id`, `since_id`, and `limit`
- send `X-Device-Token`
- reject non-200 responses

**Step 2: Parse the JSON payload**

Read:

- `messages`
- `next_cursor`
- `has_more`

Map each backend message into `MessageItem`.

**Step 3: Preserve cache merge behavior**

- update existing messages by `message_id`
- prepend new messages
- preserve `read` state for existing messages
- cap cache at `kMaxMessages`

**Step 4: Run the earlier checks again**

Run:
```bash
rg -n "kRelayMessages|relay-cursor" examples/PaperS3EDC/PaperS3EDCSync.cpp
rg -n "HTTPClient|ArduinoJson|deserializeJson" examples/PaperS3EDC/PaperS3EDCSync.cpp
```

Expected:

- fake relay seed symbols are gone
- real HTTP and JSON parsing symbols exist

### Task 4: Tighten status reporting and cursor updates

**Files:**
- Modify: `examples/PaperS3EDC/PaperS3EDCSync.cpp`

**Step 1: Update status text for the real backend**

Use short, UI-safe statuses such as:

- `消息已更新`
- `消息无更新`
- `消息拉取失败`
- `接口返回异常`

**Step 2: Update cursor behavior**

- only advance cursor after successful parsing
- preserve current cursor on request or parse failure

**Step 3: Build**

Run:
```bash
pio run -e papers3-edc
```

Expected: PASS.

### Task 5: Flash and verify against the local FastAPI backend

**Files:**
- No code changes required if previous steps pass

**Step 1: Ensure backend is running**

Run:
```bash
cd backend && ./dev.sh status
```

Expected: `api` and `worker` are running.

**Step 2: Flash the new firmware**

Run:
```bash
pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem1101
```

Expected: Upload succeeds.

**Step 3: Verify on hardware**

Expected:

- device keeps Wi-Fi connection behavior unchanged
- message page shows real DingTalk-delivered content from FastAPI
- new messages appear after polling or manual sync
- old fake relay messages no longer reappear from firmware seeds
