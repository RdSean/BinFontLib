# PaperS3 Message UI Polish Design

**Context**

The current PaperS3 EDC message experience already has real backend polling and a scrollable message stream, but three UI issues remain:

- the refresh model is automatic, yet the UI still communicates sync state with text that feels like a manual action
- the top-right status cluster overflows because page title, sync text, task counts, and battery indicator compete for the same horizontal space
- long message content can render outside the message viewport and overlap the top status bar or bottom tab bar

The goal of this round is to clarify the automatic refresh model and tighten the message page presentation without increasing the status bar height or changing the overall page structure.

**Goal**

Polish the PaperS3 message UI so the auto-refresh model is visually clear, the top status bar aligns cleanly, and long message content never draws outside the body viewport.

**Scope**

In scope:

- keep automatic polling behavior unchanged
- replace the textual sync label in the status bar with compact status icons
- shorten the message page title from `消息收件箱` to `消息`
- realign the top-right battery, task count, and sync indicators so they fit within one row
- clip message rendering to the body viewport between the top status bar and bottom tabs
- preserve existing touch-based message scrolling

Out of scope:

- adding a second status row
- adding a dedicated refresh button
- redesigning the bottom tab bar
- changing message polling intervals or backend behavior

**Approach Options**

1. Single-line status bar with icon-based sync state.

This keeps the current bar height and replaces `已同步` / `同步中` / `失败` text with compact icons drawn beside the task count and battery indicator.

Pros:

- solves the overflow without stealing vertical space
- better matches the automatic-refresh model
- smallest change to the current layout

Cons:

- requires very clear icon shapes on e-ink

2. Add a dedicated refresh hint line above the message viewport.

Pros:

- makes automatic refresh more explicit

Cons:

- reduces message viewport height
- directly conflicts with the no-extra-height requirement

3. Add a manual refresh affordance while keeping auto-polling.

Pros:

- highly discoverable

Cons:

- complicates the current touch model
- solves the wrong problem because auto-refresh already exists

**Recommendation**

Use option 1. Keep a single-row status bar, communicate sync state with icons rather than text, and fix overflow by budgeting the right-side region explicitly.

**Status Bar Design**

Left side:

- page title remains left-aligned
- message page title changes from `消息收件箱` to `消息`

Right side:

- battery icon remains right-aligned
- task count stays visible but moves into a fixed-width slot left of the battery
- sync state becomes a small icon in its own fixed-width slot left of the task count

Sync state icon mapping:

- `Idle` or successful online polling: compact refresh glyph
- `Pending`: rotating-arrows style sync glyph
- `Offline`: disconnected glyph
- `Failed`: warning glyph

The icons do not need animation in this round. They only need to be visually distinct in black-and-white e-ink rendering.

**Message Viewport Behavior**

The body viewport is already defined by `bodyTop` and `bodyBottom`. Message rendering should be hard-clipped to that rectangle before drawing the stream. Any message content above or below the viewport must not appear in the status bar or footer region.

Touch behavior remains the same:

- upper half of the message viewport scrolls up
- lower half scrolls down

The change is purely rendering safety and layout polish, not interaction redesign.

**Refresh Communication**

Automatic polling remains the only background refresh path. The UI communicates this by:

- using sync state icons in the top bar instead of manual-action text
- keeping short status line updates in the footer only when useful, without implying the user must manually refresh

This makes the device feel live without dedicating more permanent text space to refresh state.

**Testing Strategy**

Minimum verification for this round:

- firmware still builds for `papers3-edc`
- message page title and top-right status elements fit on one line without overlap
- message content no longer draws into the status bar or footer when scrolled
- automatic polling behavior remains unchanged
- message page still scrolls correctly with touch input

**Follow-up**

If the icon-only sync state proves too ambiguous on hardware, the next step should be a tiny legend in the settings page rather than expanding the status bar.
