# FastAPI DingTalk Stream Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a minimal FastAPI-based relay backend for one family and one PaperS3 device, using DingTalk Stream mode for inbound events and SQLite for local message storage.

**Architecture:** Split the backend into two runtime paths: a FastAPI HTTP process for device reads and a DingTalk Stream worker for inbound events. Keep shared persistence in SQLite and keep normalization, storage, and API concerns separated so the device only consumes a simple read-only message API.

**Tech Stack:** Python, FastAPI, Uvicorn, SQLite, SQLModel or SQLAlchemy, Pydantic, DingTalk official Python Stream SDK.

---

### Task 1: Scaffold the backend package

**Files:**
- Create: `backend/pyproject.toml`
- Create: `backend/app/__init__.py`
- Create: `backend/app/main.py`
- Create: `backend/app/config.py`
- Create: `backend/README.md`

**Step 1: Write the failing check**

Run:
```bash
test -f backend/pyproject.toml && test -f backend/app/main.py
```

Expected: FAIL because the backend package does not exist yet.

**Step 2: Add the minimal scaffold**

- Create a Python package root under `backend/`
- Add FastAPI dependencies and DingTalk Stream SDK dependency placeholders
- Add a minimal FastAPI app with `/healthz`
- Document how to run the backend locally

**Step 3: Run the scaffold check again**

Run the same `test -f ...` command.

Expected: PASS.

**Step 4: Run the app**

Run:
```bash
cd backend && python -m uvicorn app.main:app --reload
```

Expected: FastAPI starts and `/healthz` responds successfully.

**Step 5: Commit**

```bash
git add backend
git commit -m "feat: scaffold fastapi dingtalk backend"
```

### Task 2: Add database and message model

**Files:**
- Create: `backend/app/db.py`
- Create: `backend/app/models.py`
- Create: `backend/app/schemas.py`
- Modify: `backend/app/main.py`

**Step 1: Write the failing check**

Run:
```bash
rg -n "class Message|source_event_id|sent_at|raw_payload" backend/app
```

Expected: No message model exists yet.

**Step 2: Implement the minimal persistence layer**

- Add SQLite connection setup
- Add `Message` model with:
  - `id`
  - `source_event_id`
  - `source_type`
  - `title`
  - `body`
  - `sent_at`
  - `raw_payload`
  - `created_at`
- Create tables on startup

**Step 3: Run the check again**

Run the same `rg` command.

Expected: The message model and fields exist.

**Step 4: Run the app**

Run:
```bash
cd backend && python -m uvicorn app.main:app --reload
```

Expected: App starts and creates the SQLite schema.

**Step 5: Commit**

```bash
git add backend/app/db.py backend/app/models.py backend/app/schemas.py backend/app/main.py
git commit -m "feat: add backend sqlite message model"
```

### Task 3: Add message storage service and dedupe behavior

**Files:**
- Create: `backend/app/services/__init__.py`
- Create: `backend/app/services/message_service.py`
- Create: `backend/tests/test_message_service.py`

**Step 1: Write the failing test**

```python
def test_save_message_deduplicates_source_event_id():
    ...
```

**Step 2: Run the test to verify it fails**

Run:
```bash
cd backend && pytest backend/tests/test_message_service.py -q
```

Expected: FAIL because the service does not exist yet.

**Step 3: Implement the minimal service**

- Add a `save_message()` helper
- Add dedupe by `source_event_id`
- Add `list_messages_after()` for cursor reads

**Step 4: Run the test again**

Run:
```bash
cd backend && pytest backend/tests/test_message_service.py -q
```

Expected: PASS.

**Step 5: Commit**

```bash
git add backend/app/services backend/tests/test_message_service.py
git commit -m "feat: add backend message storage service"
```

### Task 4: Add the device read API

**Files:**
- Create: `backend/app/api/__init__.py`
- Create: `backend/app/api/device.py`
- Modify: `backend/app/main.py`
- Create: `backend/tests/test_device_api.py`

**Step 1: Write the failing test**

```python
def test_get_messages_requires_device_token():
    ...

def test_get_messages_returns_incremental_messages():
    ...
```

**Step 2: Run the test to verify it fails**

Run:
```bash
cd backend && pytest backend/tests/test_device_api.py -q
```

Expected: FAIL because the API does not exist yet.

**Step 3: Implement the minimal API**

- Add `GET /api/device/messages`
- Require `X-Device-Token`
- Accept `device_id`, `since_id`, `limit`
- Return `messages`, `next_cursor`, `has_more`, `server_time`

**Step 4: Run the tests again**

Run:
```bash
cd backend && pytest backend/tests/test_device_api.py -q
```

Expected: PASS.

**Step 5: Commit**

```bash
git add backend/app/api backend/app/main.py backend/tests/test_device_api.py
git commit -m "feat: add backend device message api"
```

### Task 5: Integrate DingTalk Stream worker

**Files:**
- Create: `backend/app/integrations/__init__.py`
- Create: `backend/app/integrations/dingtalk_stream_client.py`
- Create: `backend/app/services/dingtalk_service.py`
- Create: `backend/run_stream_worker.py`
- Create: `backend/tests/test_dingtalk_normalization.py`

**Step 1: Write the failing test**

```python
def test_normalize_dingtalk_event_to_message_payload():
    ...
```

**Step 2: Run the test to verify it fails**

Run:
```bash
cd backend && pytest backend/tests/test_dingtalk_normalization.py -q
```

Expected: FAIL because the normalization layer does not exist yet.

**Step 3: Implement the minimal integration**

- Add a normalization helper that converts a DingTalk event into:
  - `source_event_id`
  - `title`
  - `body`
  - `sent_at`
  - `source_type`
- Add a stream worker script that:
  - creates the SDK client
  - registers the event callback
  - saves normalized messages
- Keep the first version narrow and text-only

**Step 4: Run the test again**

Run:
```bash
cd backend && pytest backend/tests/test_dingtalk_normalization.py -q
```

Expected: PASS.

**Step 5: Manual worker smoke check**

Run:
```bash
cd backend && python run_stream_worker.py
```

Expected: Worker starts, connects, and logs Stream lifecycle events.

**Step 6: Commit**

```bash
git add backend/app/integrations backend/app/services/dingtalk_service.py backend/run_stream_worker.py backend/tests/test_dingtalk_normalization.py
git commit -m "feat: add dingtalk stream worker"
```

### Task 6: Add backend config and runtime docs

**Files:**
- Modify: `backend/app/config.py`
- Modify: `backend/README.md`
- Create: `backend/.env.example`

**Step 1: Write the failing check**

Run:
```bash
rg -n "DEVICE_TOKEN|DINGTALK_CLIENT_ID|DINGTALK_CLIENT_SECRET|DATABASE_URL" backend
```

Expected: Runtime configuration docs are incomplete.

**Step 2: Implement the minimal runtime config**

- Add strongly named config fields
- Add `.env.example`
- Document how to run:
  - FastAPI service
  - Stream worker
- Document local SQLite path

**Step 3: Run the check again**

Run the same `rg` command.

Expected: Config variables are documented and wired.

**Step 4: Commit**

```bash
git add backend/app/config.py backend/README.md backend/.env.example
git commit -m "docs: add backend runtime config"
```

### Task 7: Add end-to-end local verification

**Files:**
- Create: `backend/tests/test_end_to_end_cursor_flow.py`
- Optionally modify: `backend/app/services/message_service.py`
- Optionally modify: `backend/app/api/device.py`

**Step 1: Write the failing test**

```python
def test_cursor_flow_returns_only_new_messages():
    ...
```

**Step 2: Run the test to verify it fails**

Run:
```bash
cd backend && pytest backend/tests/test_end_to_end_cursor_flow.py -q
```

Expected: FAIL until the full path is wired consistently.

**Step 3: Implement the minimal fixes**

- Ensure cursor semantics are stable
- Ensure API ordering is deterministic
- Ensure `next_cursor` matches the last returned row

**Step 4: Run the full backend test suite**

Run:
```bash
cd backend && pytest backend/tests -q
```

Expected: PASS.

**Step 5: Commit**

```bash
git add backend/tests backend/app/services/message_service.py backend/app/api/device.py
git commit -m "test: verify backend message cursor flow"
```
