# PaperS3 Poll Interval Migration Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Make the PaperS3 EDC device use `2s` charging polling and `5s` battery polling by default, while auto-migrating legacy persisted interval values on boot.

**Architecture:** Add a hardware-independent poll-policy helper that owns default values and migration rules, then call it from device boot after `loadState`. Keep the message polling loop and settings page wiring intact so only the loaded values change.

**Tech Stack:** C++17, PlatformIO Arduino app, pytest, host-side compiler harness

---

### Task 1: Add the failing tests

**Files:**
- Create: `tests/test_papers3_edc_poll_policy.py`
- Modify: `examples/PaperS3EDC/PaperS3EDCModel.h`
- Create: `examples/PaperS3EDC/PaperS3EDCPollPolicy.h`

**Step 1: Write the failing test**

Write a pytest that:

- Reads `examples/PaperS3EDC/PaperS3EDCModel.h`
- Asserts the default charging interval is `2`
- Asserts the default battery interval is `5`
- Asserts `examples/PaperS3EDC/PaperS3EDCPollPolicy.h` exists
- Compiles and runs a small C++ harness that checks migration behavior

**Step 2: Run test to verify it fails**

Run: `backend/.venv/bin/pytest tests/test_papers3_edc_poll_policy.py -v`

Expected: FAIL because the battery default is still `10` and the poll-policy helper does not exist yet.

### Task 2: Implement the poll-policy helper

**Files:**
- Create: `examples/PaperS3EDC/PaperS3EDCPollPolicy.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCModel.h`
- Modify: `examples/PaperS3EDC/PaperS3EDCApp.cpp`

**Step 1: Write minimal implementation**

Add a header-only helper that defines:

- `kDefaultChargingPollIntervalSec = 2`
- `kDefaultBatteryPollIntervalSec = 5`
- `normalizePollIntervals(uint32_t& chargingSec, uint32_t& batterySec)`

Migration rules:

- `charging=4, battery=0` becomes `2,5`
- `charging=2, battery=10` becomes `2,5`
- Any value below the minimum is repaired to its default side only
- Otherwise keep the user-defined values as-is

Update `AppState` defaults to use the new constants and call normalization from device boot after `loadState`. If normalization changes the state, persist it immediately.

**Step 2: Run the test to verify it passes**

Run: `backend/.venv/bin/pytest tests/test_papers3_edc_poll_policy.py -v`

Expected: PASS

### Task 3: Run focused verification

**Files:**
- Test: `tests/test_papers3_edc_poll_policy.py`

**Step 1: Run focused tests**

Run: `backend/.venv/bin/pytest tests/test_papers3_edc_poll_policy.py -v`

Expected: PASS

**Step 2: Build the device environment**

Run: `pio run -e papers3-edc`

Expected: SUCCESS
