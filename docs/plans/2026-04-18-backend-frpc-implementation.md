# Backend FRPC Exposure Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add TCP-based `frpc` exposure to the backend dev workflow so `backend/dev.sh start` also publishes the local API through FRPS.

**Architecture:** Extend `backend/dev.sh` to manage a third background process for `frpc`, backed by a checked-in `backend/frpc.toml` template. Keep all runtime state in `backend/run/` and fail fast when prerequisites or config validation fail.

**Tech Stack:** zsh, frp (`frpc` 0.68.x TOML config), pytest, subprocess

---

### Task 1: Add the failing test

**Files:**
- Create: `backend/tests/test_dev_script.py`
- Modify: `backend/dev.sh`
- Create: `backend/frpc.toml`

**Step 1: Write the failing test**

Write a pytest that:

- Creates an isolated temp `backend/` directory
- Copies `backend/dev.sh` into it
- Creates stub `.venv/bin/python`
- Creates `.env.dingtalk`
- Creates `frpc.toml`
- Prepends a fake `frpc` executable to `PATH`
- Runs `./dev.sh start`, `./dev.sh status`, then `./dev.sh stop`
- Asserts `frpc.pid` is created and later removed

**Step 2: Run test to verify it fails**

Run: `cd backend && .venv/bin/pytest tests/test_dev_script.py -v`

Expected: FAIL because the current script does not create or report any `frpc` process.

### Task 2: Implement the minimal shell changes

**Files:**
- Modify: `backend/dev.sh`
- Create: `backend/frpc.toml`
- Modify: `backend/README.md`

**Step 1: Write minimal implementation**

Update `backend/dev.sh` to:

- Track `frpc` pid and log files
- Verify `frpc` exists and the config file is present
- Run `frpc verify -c backend/frpc.toml` before start
- Start `frpc` in background after API and worker
- Include `frpc` in `stop`, `status`, `logs`, and usage output
- Roll back started services if `frpc` fails to come up

Add a simple `backend/frpc.toml` template using a TCP proxy from `127.0.0.1:8000`.

Document the new behavior in `backend/README.md`.

**Step 2: Run the test to verify it passes**

Run: `cd backend && .venv/bin/pytest tests/test_dev_script.py -v`

Expected: PASS

### Task 3: Run focused verification

**Files:**
- Test: `backend/tests/test_dev_script.py`
- Test: `backend/tests/test_stream_worker.py`

**Step 1: Run focused tests**

Run: `cd backend && .venv/bin/pytest tests/test_dev_script.py tests/test_stream_worker.py -v`

Expected: PASS

**Step 2: Manual config validation**

Run: `frpc verify -c backend/frpc.toml`

Expected: Syntax ok, even with placeholder values.
