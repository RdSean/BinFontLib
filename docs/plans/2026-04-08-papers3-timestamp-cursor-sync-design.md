# PaperS3 Timestamp Cursor Sync Design

**Context**

The current message sync path uses a database auto-increment `id` as the device cursor. That is fragile because the SQLite database file can be recreated or replaced, which resets message IDs while the device still remembers an older, larger cursor value. When that happens, the device keeps polling with a stale `since_id` and never receives new messages again.

There is also a configuration risk: API and worker must always use the same database file. If they diverge, messages can be saved to one database while the device API reads from another.

**Goal**

Make device message sync resilient by unifying backend database configuration and replacing the device cursor from numeric message IDs to a trusted message timestamp cursor.

**Scope**

In scope:

- use a single configured `DATABASE_URL` source for both API and worker
- remove runtime reliance on SQLite auto-increment IDs for device sync cursors
- expose device message pagination using message timestamps
- store the returned timestamp cursor on the device
- handle the device “no cursor yet” case as an initial pull

Out of scope:

- changing DingTalk message ingestion semantics beyond cursor support
- using device-local time as part of sync correctness
- changing task sync

**Approach Options**

1. Use backend `sent_at` as the cursor, serialized as an ISO string.

Pros:

- stable across database rebuilds
- readable in logs and persisted device state
- based on a message timestamp already present in the system

Cons:

- requires careful handling when multiple messages share the same timestamp

2. Use backend `created_at` as the cursor.

Pros:

- always available at insert time

Cons:

- less semantically tied to actual DingTalk message time
- can drift from the real source event time

3. Keep numeric IDs but add reset heuristics.

Pros:

- smaller change set

Cons:

- still fundamentally fragile when databases are replaced
- only treats the symptom

**Recommendation**

Use option 1. Cursor on backend `sent_at`, encoded as an ISO timestamp string, and persist that string directly on the device.

**Cursor Contract**

Request:

- device sends `since_ts=<ISO timestamp>` when it has a saved cursor
- if the device has no cursor, it omits `since_ts`

Response:

- backend returns `messages[]`
- backend returns `next_cursor` as an ISO timestamp string or an empty string when no cursor is available
- backend may continue returning `has_more`

Cursor advancement:

- device only updates local cursor after successful parsing
- backend advances cursor to the latest delivered message timestamp
- if no new messages are returned, backend can echo the incoming cursor

**Handling Equal Timestamps**

If multiple messages can share the same `sent_at`, pure `>` filtering on timestamp risks skipping or duplicating edge cases. The practical first pass is:

- order by `sent_at`, then `id`
- query messages strictly after the provided timestamp
- accept that equal-timestamp collisions are rare for current DingTalk usage

If collisions become real, the next step is a compound cursor such as `sent_at + id`.

**No-Cursor Behavior**

If the device has no saved cursor:

- it performs an initial pull without `since_ts`
- backend returns the latest batch under the normal limit
- device persists the returned `next_cursor`

This avoids relying on device RTC, which is not trustworthy after resets.

**Database Configuration**

The backend must have a single authoritative database path:

- `DATABASE_URL` from `backend/.env.dingtalk`
- API and worker both resolve through the same config loader
- runtime defaults should not silently point to a different relative path during normal operation

**Testing Strategy**

Minimum verification:

- worker and API are provably using the same configured database file
- a newly ingested DingTalk message appears through the device API when queried with an older timestamp cursor
- a device with no cursor can pull messages successfully
- device persists and reuses the returned timestamp cursor
- restarting with a rebuilt database no longer causes permanent cursor deadlock due to numeric ID mismatch

**Follow-up**

If timestamp-only cursors ever prove ambiguous, upgrade the API contract to a compound cursor of `sent_at + id` while keeping the same device persistence pattern.
