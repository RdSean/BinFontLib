# PaperS3 Partial Refresh Design

**Context**

The current PaperS3 message experience now has:

- chronological message paging
- left/right tap navigation
- top-bar page indicators
- automatic jump to the last page when new messages arrive

However, the rendering pipeline is still fundamentally full-screen:

- the renderer draws directly into the display surface
- `setClipRect()` is used only for draw clipping, not e-ink partial refresh
- frame submission still ends with a full `_display->display()`

That means the app does not yet have a real partial-refresh architecture. The next step needs to address two distinct concerns:

1. **Drawing foundation**: how regions are rendered and committed
2. **Refresh strategy**: when to do small partial updates vs. larger refreshes vs. full refreshes

**Goal**

Add a partial-refresh foundation for PaperS3 that allows the app to refresh only the status bar and the message viewport when appropriate, without repainting the footer, while retaining a controlled path back to periodic full refreshes.

**Scope**

In scope:

- define app-level refresh regions
- introduce a mixed region-canvas + dirty-rect rendering model
- define refresh modes for message arrival, paging, and status changes
- define a full-refresh threshold policy for e-ink cleanup
- keep the footer static during message-only updates

Out of scope:

- redesigning message layout again
- changing backend APIs
- implementing backend-side long-message splitting
- optimizing non-message pages beyond basic compatibility

**Current Constraints**

- The inbox renderer draws directly onto the display surface today.
- The project already has access to `M5Canvas` / `LGFX_Sprite` style off-screen rendering through the existing font stack.
- Footer buttons should not refresh when only message content or page indicators change.
- The status bar page indicator must refresh when page count or page index changes.

**Approach Options**

1. Mixed region canvas + dirty rect.

Use semantic screen regions and render only changed regions. Small/simple areas may still be drawn directly, but message viewport updates are rendered into a dedicated off-screen canvas before being pushed.

Pros:

- strongest correctness for complex text regions
- good fit for the message viewport
- avoids unnecessary footer repaint
- scales well to message append behavior

Cons:

- requires new render-target abstractions
- needs region-level refresh orchestration

2. Dirty rect only, no canvas.

Draw directly to the display for every dirty rectangle.

Pros:

- lower implementation cost
- less extra memory

Cons:

- much more fragile for text-heavy regions
- harder to guarantee correct erase/repaint behavior
- more likely to leave ghosting or stale pixels

3. Full off-screen page composition.

Render entire page surfaces off-screen and push them as needed.

Pros:

- simplest correctness model

Cons:

- heaviest memory cost
- unnecessary for footer/static regions

**Recommendation**

Use option 1.

The message viewport is text-heavy and layout-sensitive, which makes direct dirty-rect drawing too fragile. A semantic region model with region canvases where needed gives a cleaner path:

- `status bar`: small region, can be direct or canvas-backed
- `message viewport`: canvas-backed region
- `footer`: static, excluded from message-triggered refreshes

**Refresh Regions**

The screen should be split into fixed semantic zones:

1. `StatusBarRegion`
   Includes:
   - page title
   - page count / page index label
   - sync icon
   - battery icon
   - task progress indicator

2. `MessageViewportRegion`
   Includes:
   - the paged message body area only

3. `FooterRegion`
   Includes:
   - bottom tabs
   - bottom status text

Footer remains outside message-triggered partial updates unless a footer-specific action actually changes it.

**Rendering Model**

Rendering should become mode-driven rather than always “draw everything, then display”.

Recommended model:

- full-page render path remains available
- message viewport gets its own off-screen canvas
- status bar can start with direct redraw into a bounded dirty region, or use a small canvas if needed for consistency
- dirty rectangles are semantic, not tiny glyph-level fragments

That means:

- do not refresh per-character
- do not try to patch arbitrary micro-rectangles inside flowing text
- do refresh whole semantic slices such as “status bar” or “message viewport”

**Refresh Modes**

Define explicit refresh intents:

1. `FullRefresh`
   Used for:
   - first page load
   - app startup
   - page/system transitions that invalidate large parts of the UI
   - periodic cleanup after many partial updates

2. `StatusOnlyRefresh`
   Used for:
   - page indicator change without message viewport change
   - sync icon / battery / task-progress updates when message body is unchanged

3. `MessagePageRefresh`
   Used for:
   - left/right page navigation
   - automatic jump to a different message page

   Refreshes:
   - `StatusBarRegion`
   - `MessageViewportRegion`

   Does not refresh:
   - `FooterRegion`

4. `MessageAppendRefresh`
   Used for:
   - new message arrival when the visible page changes or last page content changes

   Refreshes:
   - `StatusBarRegion`
   - `MessageViewportRegion`

   Does not refresh:
   - `FooterRegion`

**Threshold Policy**

E-ink partial updates need a cleanup path.

Recommended policy for the first version:

- count message/status partial refreshes
- after a configurable threshold, perform one `FullRefresh`
- reset the counter after full refresh

Good starting point:

- every `8-12` message/status partial refreshes, force a full refresh

This threshold should be a named constant, not hardcoded inline.

**Fast vs. Quality**

Current renderer behavior is closer to quality-oriented text rendering than a tuned fast-refresh path.

The first version of partial refresh should prioritize:

- correctness of region contents
- stable ghosting behavior
- no footer involvement during message-only updates

Only after that should we optimize for speed further.

**Message Arrival Strategy**

When a new message arrives:

- update page metrics
- jump to last page if required
- redraw the message viewport region
- redraw the status bar if page count or current page changed
- leave footer untouched

This gives the user-visible effect you want:

- only the newly relevant content area changes
- page count updates are visible
- bottom buttons do not blink or flash

**Implementation Notes**

- Introduce a refresh coordinator layer rather than sprinkling partial-refresh decisions inside unrelated view logic.
- Keep the full-screen render path for fallback and recovery.
- Prefer semantic dirty regions over tiny micro-rectangles.
- For the message viewport, use a canvas-backed redraw of the whole region rather than trying to surgically patch individual lines of text.

**Testing Strategy**

Minimum verification for this round:

- full refresh still works across all pages
- page navigation refreshes status bar + message viewport only
- new message arrival refreshes status bar + message viewport only
- footer remains visually stable during message-only updates
- periodic forced full refresh still restores a clean display

**Follow-up**

Once the regional refresh pipeline is stable, the next refinement can explore whether the status bar should remain direct-rendered or also move to a tiny canvas for more deterministic compositing.
