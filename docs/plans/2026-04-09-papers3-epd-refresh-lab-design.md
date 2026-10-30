# PaperS3 EPD Refresh Lab Design

**Context**

The current PaperS3 inbox firmware has reached a stable refresh baseline:

- redraw only the status bar and message viewport logically
- still submit the frame with full-screen `display()`
- avoid unstable hardware region submission in the main inbox path

Previous attempts to use `display(x, y, w, h)` directly inside the inbox caused dropped content and mosaic artifacts. That means the project needs a controlled, isolated way to validate EPD partial-refresh behavior before reintroducing region commits into the message UI.

At the same time, the settings page still contains a large font diagnostics block that is no longer useful for normal device operation and consumes most of the remaining screen space.

**Goal**

Remove the font diagnostics block from the settings page and replace it with a dedicated EPD refresh lab entry, so hardware partial-refresh behavior can be tested in isolation on-device.

**Scope**

In scope:

- remove font diagnostics content from the settings page
- add a `刷新实验` entry to settings
- add a dedicated refresh lab page
- allow testing fixed status-area and body-area partial refreshes
- allow switching draw path between direct drawing and canvas-backed drawing
- reuse the existing persisted EPD mode setting for experiments

Out of scope:

- changing inbox rendering behavior again
- reintroducing hardware partial refresh into the message page
- redesigning the whole settings page
- adding a generic diagnostics framework

**Approach Options**

1. Dedicated refresh lab page from settings.

Pros:

- isolates unstable hardware behavior from the production inbox
- easy to reason about failures
- keeps settings page simple after removing diagnostics

Cons:

- adds one more page to the app flow

2. Inline refresh lab block inside settings.

Pros:

- less navigation

Cons:

- mixes risky experiments with regular settings
- harder to keep layout readable

**Recommendation**

Use option 1.

The refresh lab is diagnostic in nature and should not share rendering flow with the settings list itself. A dedicated page makes it easier to test, reason about, and discard failed experiments without destabilizing the rest of the UI.

**Settings Page Changes**

- Remove the bottom `字体诊断` block entirely
- Keep the existing list-style settings layout
- Add one new row:
  - `刷新实验`

This returns the settings page to a compact operational menu.

**Refresh Lab Page**

Add a new page dedicated to minimal EPD refresh experiments.

The page should display:

- current persisted EPD mode
- current draw path:
  - `Direct`
  - `Canvas`
- a small top test rectangle
- a larger body test rectangle
- a short instruction line

The lab should support:

- toggling draw path
- triggering a status-area region refresh
- triggering a body-area region refresh
- returning to settings

**Interaction Model**

Keep touch semantics simple and fixed:

- top test rectangle: trigger status-area refresh test
- body test rectangle: trigger body-area refresh test
- one control row toggles draw path
- one control row returns to settings

No scrolling, no nested state, no dynamic layouts.

**Rendering Strategy**

The lab should support two rendering paths for the same rectangles:

1. `Direct`
   Draw directly onto the display in the target region, then commit the region.

2. `Canvas`
   Render the target region into an `M5Canvas`, push the sprite into place, then commit the region.

This isolates the key question:

- is instability caused by the region commit itself
- or by the combination of canvas rendering and region commit

**State Model**

Add a small refresh-lab state to app state:

- current draw path
- test counter / label for status rect
- test counter / label for body rect

This state can be transient and does not need persistence.

**Success Criteria**

- settings page no longer shows the font diagnostics block
- settings page includes a `刷新实验` entry
- refresh lab page can trigger repeatable region refresh tests
- refresh lab can switch between direct and canvas drawing
- the existing EPD mode selector continues to control the hardware mode used by the lab
