# PaperS3 Message Minimal Layout Design

**Context**

The PaperS3 message page has already been simplified to a clipped scrollable stream with a tighter top bar, but the message cards still carry metadata that is no longer useful in the on-device reading flow:

- the sender line is redundant for the current single-source use case
- the `relay://...` source line wastes horizontal and vertical space
- time currently shares the same visual row budget as sender text, which risks overlap when the content becomes dense

The device now also has updated font assets in LittleFS:

- `LXGWWenKaiLite-for-edc-28.bin`
- `LXGWWenKaiLite-for-edc-32.bin`

The updated `edc-32` replaces the previous `霞鹜文楷32.bin` path for large text rendering.

**Goal**

Reduce each message item to the minimum useful reading layout: a compact time column in 28 pt and a body column in 32 pt, with no sender line and no source line.

**Scope**

In scope:

- remove sender-name display from the message list
- remove `relay://...` source display from the message list
- reserve a fixed right-side time column that never overlaps the body text
- render message time in the smaller 28 pt style
- render message body in the larger 32 pt style
- preserve scrolling, clipping, unread marker behavior, and automatic polling

Out of scope:

- changing backend message fields
- changing message storage shape
- changing non-message pages to use the large body font
- redesigning the settings or progress pages

**Approach Options**

1. Fixed two-column message row: time on the right, body on the left.

Each message item reserves a narrow time column and lets body text wrap only inside the remaining content width.

Pros:

- guarantees no time/body overlap
- simple to reason about on e-ink
- scales well with long messages

Cons:

- slightly reduces maximum body width

2. Floating timestamp above the body.

Pros:

- gives the body full width

Cons:

- increases vertical height
- visually reintroduces a metadata row

3. Timestamp inline with the first body line.

Pros:

- very compact in theory

Cons:

- fragile with longer body text
- higher risk of overlap and uneven wrapping

**Recommendation**

Use option 1. Keep the timestamp isolated in a fixed right-side slot and let the body occupy the remaining left-side width. This is the safest and cleanest layout for dense message text on e-ink.

**Message Row Design**

Per message item:

- no sender line
- no source line
- body becomes the primary content block
- timestamp sits in a fixed-width right column aligned to the top of the item
- unread dot remains at the start of the body column for now

Body column:

- uses the larger 32 pt message font
- wraps inside the left content region only
- never crosses into the timestamp column

Time column:

- uses the smaller 28 pt message/status font
- uses an extremely compact format such as `08:14`
- stays visually detached from the body text

The timestamp should be derived from the existing `sent_at` field and normalized to a short on-device display string if the raw value is an ISO timestamp.

**Typography Direction**

- message body: 32 pt font from LittleFS
- top status bar and compact metadata such as timestamp: 28 pt font from LittleFS

This keeps the reading hierarchy strong: message content remains dominant, while system chrome and time metadata stay secondary.

**Rendering Notes**

- message height measurement must account for the new narrower body width after subtracting the timestamp column
- clipping remains mandatory so large 32 pt text never bleeds into the status bar or footer
- the old `messageSender()` and source-line display path should be removed from the message list rendering if no longer used elsewhere

**Testing Strategy**

Minimum verification for this round:

- firmware builds with the updated layout
- message page shows only time and body text per item
- long message bodies do not overlap the time column
- long message bodies still stay clipped to the message viewport
- 32 pt body text and 28 pt time/status text render correctly from LittleFS assets

**Follow-up**

If the denser 32 pt body rendering reduces messages-per-screen too much, the next step should be tuning message spacing and time-column width rather than reintroducing sender/source metadata.
