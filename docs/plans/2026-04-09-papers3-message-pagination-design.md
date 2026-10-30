# PaperS3 Message Pagination Design

**Context**

The current message page already uses a simplified time-plus-body layout and clipped viewport rendering, but interaction is still modeled as a vertically scrollable stream:

- messages are visually inserted with newest items at the top
- touch interaction uses upper/lower screen areas to move through a scroll offset
- the footer status line carries paging feedback

That behavior does not match the desired reading model for later partial-refresh work. The new target is closer to a page-based e-reader:

- older messages stay at the top
- newer messages accumulate toward the bottom
- once the visible page is full, additional content flows onto the next page
- when new messages arrive, the device should jump to the last page automatically
- page navigation should use left/right touch regions rather than vertical scroll zones

**Goal**

Turn the message page into a chronological paged reader: old messages first, new messages last, fixed-height pages, left/right tap navigation, and top-status-bar paging feedback.

**Scope**

In scope:

- change message rendering order to chronological display
- replace scroll-offset navigation with explicit page indexing
- paginate message content by visible viewport height
- auto-jump to the last page when new messages arrive
- move page feedback into the top status bar
- update touch handling so left/right areas map to previous/next page

Out of scope:

- backend message ordering or API changes
- partial refresh implementation itself
- changes to non-message pages
- redesign of the bottom tab bar

**Approach Options**

1. Precompute explicit message pages before rendering.

Each render pass measures messages in chronological order, groups them into fixed-height pages, and renders only the current page.

Pros:

- page boundaries are stable and easy to reason about
- directly supports future partial refresh on a per-page basis
- new messages naturally fill the bottom of the last page

Cons:

- requires a small layout model instead of direct stream painting

2. Keep a continuous stream but move by one viewport height per tap.

Pros:

- lower implementation cost

Cons:

- not true pagination
- page boundaries drift with content changes
- weaker fit for future partial refresh

3. Treat each message as a separate page unit.

Pros:

- simple navigation logic

Cons:

- wastes screen space
- breaks the desired “fill remaining blank area first” reading behavior

**Recommendation**

Use option 1. Precompute the visible pages from a chronological message stream and render one page at a time. This best matches the long-term partial-refresh direction and gives predictable reading behavior.

**Message Order**

The on-device message list should be displayed in ascending time order:

- oldest message at the top of the first page
- newest message at the bottom of the last page

This can be achieved either by changing merge/storage order or by rendering the existing storage array in reverse. The preferred implementation is to keep storage behavior simple and make the renderer/controller interpret messages chronologically in one place, unless a storage-side change clearly reduces complexity.

**Pagination Model**

The message body viewport remains bounded by the existing `bodyTop` and `bodyBottom` layout limits.

Pagination rules:

- layout starts at the top of page 1
- messages are added in chronological order
- if the next message fits, place it below the previous one
- if it does not fit, start a new page and place it at that page’s top
- the last page may have blank space at the bottom until new messages arrive

This produces the desired “new messages gradually occupy remaining blank area” effect.

**Auto-Advance Behavior**

When polling merges in at least one new message:

- recompute pagination
- set the current message page to the last page
- render the last page immediately if the message page is visible

This guarantees that fresh messages appear without requiring manual navigation.

**Interaction Model**

The message page becomes page-based rather than scroll-based:

- tap left half of the message body viewport: previous page
- tap right half of the message body viewport: next page
- taps outside the body viewport do nothing beyond existing tab/status interactions

There is no vertical scroll gesture in this version.

**Status Feedback**

Paging feedback moves from the bottom status line to the top status bar. The message-page title region should include concise page state such as:

- `1/3`
- `末页`
- or both, if space allows

The footer status line can remain available for generic sync/task messages, but message navigation feedback should no longer depend on it.

**Rendering Notes**

- clipping remains mandatory within the message viewport
- per-page drawing should still use the 32 pt body font and 28 pt compact font
- unread dot behavior may remain unchanged
- the page indicator should use the compact 28 pt path

**Testing Strategy**

Minimum verification for this round:

- firmware builds successfully
- messages display old-to-new across pages
- new messages move the view to the last page
- tapping left/right body regions changes pages correctly
- top status bar shows page feedback for the message page
- content still stays clipped to the message viewport

**Follow-up**

Once pagination is stable, the next logical step is to make the renderer expose page bounds suitable for partial refresh of only the last page or only the page indicator.
