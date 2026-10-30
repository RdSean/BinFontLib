# PaperS3 Real Message Sync Design

**Context**

The PaperS3 EDC device already has:

- a local message cache in `AppState`
- persisted message cursor and polling intervals in `LittleFS`
- a message stream UI that can render variable-height messages
- a FastAPI backend that now exposes a real message API at `/api/device/messages`

What is still fake is the device-side message sync path in `PaperS3EDCSyncService`. It currently reads from in-firmware relay seeds instead of performing a real HTTP request.

**Goal**

Replace only the message pull path with a real HTTP fetch against the local FastAPI backend running at `http://192.168.3.181:8000/api/device/messages`, while keeping the task sync path and the rest of the PaperS3 app behavior unchanged.

**Scope**

In scope:

- replace fake message relay seeds with a real HTTP GET
- send `device_id`, `since_id`, and `limit` query parameters
- send `X-Device-Token`
- parse the backend JSON response
- merge returned messages into the existing local cache
- update the local cursor from `next_cursor`
- preserve current polling, persistence, and message stream rendering

Out of scope:

- making backend host configurable in the UI
- replacing fake task sync
- supporting message reply or ack
- supporting attachments or rich media

**Approach Options**

1. Hardcode the development backend address now, add configurability later.

This uses a fixed host, port, and device token in firmware for the first integration pass.

Pros:

- fastest path to a real end-to-end demo
- smallest change set
- easiest to debug

Cons:

- requires firmware change if the backend host changes

2. Add backend host configuration immediately.

This stores host and port in `DeviceConfig` and exposes them on-device.

Pros:

- more flexible

Cons:

- expands scope into config UI, persistence, and validation before the real chain is proven

3. Keep fake sync and add a compile-time switch for real backend mode.

Pros:

- keeps the fake demo path available

Cons:

- adds branching and complexity before it is needed

**Recommendation**

Use option 1 for this round. Keep the real backend endpoint fixed in firmware, prove the DingTalk-to-device chain, and only then expose backend configuration if it is still needed.

**HTTP Contract**

Request:

- `GET /api/device/messages`
- query:
  - `device_id=papers3-demo`
  - `since_id=<cursor as integer>`
  - `limit=8`
- header:
  - `X-Device-Token: paper-s3-local-token`

Response fields used by device:

- `messages[]`
  - `id`
  - `title`
  - `body`
  - `sent_at`
- `next_cursor`
- `has_more`

The device can ignore `source_event_id`, `source_type`, `raw_payload`, and `server_time` for now.

**Device Mapping**

The existing `MessageItem` model remains the render model.

Mapping:

- `message_id` <- stringified backend `id`
- `title` <- backend `title`
- `body` <- backend `body`
- `sent_at` <- backend `sent_at`
- `source` <- fixed `relay://dingtalk`
- `read` <- `false` for new messages, preserve previous read state if message already exists

Cursor handling:

- store backend `next_cursor` as a string in `lastMessageCursor`
- parse it back to integer when issuing the next request

**Error Handling**

- if Wi-Fi is disconnected, keep existing offline behavior
- if HTTP connection fails, set sync state to failed or offline and keep local cache intact
- if JSON parsing fails, keep local cache intact and expose a short status
- if API returns no new messages, keep cursor unchanged or set it to `next_cursor` if provided

No destructive behavior is allowed on message pull failure.

**Testing Strategy**

Minimum verification for this round:

- firmware compiles with HTTP and JSON dependencies
- device can request the API on the local network
- API response is parsed and merged into local cache
- cursor advances after a successful pull
- no regression to message rendering or polling loop

**Follow-up**

After this is stable, the next sensible step is to move backend host, port, token, and optionally `device_id` into persisted device configuration.
