# PaperS3 EPD Refresh Analysis And Plan

**Context**

The PaperS3 inbox now has a dedicated EPD refresh lab. Using that lab, we compared:

- `Text + Direct`
- `Text + Canvas`
- `Fast + Direct`
- `Fast + Canvas`
- `Quality` region refresh behavior

We also inspected the M5GFX EPD driver implementation in:

- `.pio/libdeps/papers3-edc/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp`
- `.pio/libdeps/papers3-edc/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.hpp`
- `.pio/libdeps/papers3-edc/M5GFX/src/lgfx/v1/panel/Panel_HasBuffer.cpp`
- `.pio/libdeps/papers3-edc/M5GFX/src/lgfx/v1/LGFXBase.cpp`

This document records the findings and the recommended path forward.

---

## 1. Confirmed Findings

### 1.1 The project uses `Panel_EPD`

PaperS3 runs through M5GFX's EPD panel implementation. The firmware is not using a custom e-ink driver. All refresh behavior ultimately goes through `Panel_EPD`.

### 1.2 LUTs in `Panel_EPD` are waveform schedules, not simple palettes

The LUT tables define how each 4-bit grayscale level should be driven over time.

Conceptually:

- horizontal axis = grayscale level
- vertical axis = time step
- table value meanings:
  - `1`: drive toward black
  - `2`: drive toward white
  - `3`: no-op
  - `0`: end

So `quality`, `text`, `fast`, and `fastest` are not just “speed presets”; they are different multi-step electrophoretic driving programs.

### 1.3 `Fast + Canvas` is the best current region-refresh combination

Observed behavior:

- `Fast + Canvas` only refreshes changed text effectively
- `Fast + Direct` refreshes all text inside the target rectangle
- `Text + Canvas` and `Text + Direct` are slower and tend to refresh the full rectangle content rather than only changed glyphs

This makes `Fast + Canvas` the best current candidate for production message partial refresh.

### 1.4 The earlier “wrong vertical rectangle refresh” was a coordinate bug

The main root cause was not the LUT itself.

The issue came from region submission:

- normal drawing paths in `Panel_EPD` rotate coordinates before tracking dirty ranges
- `Panel_EPD::display(x, y, w, h)` did not rotate the input region the same way

That caused logical screen coordinates to be submitted as if they were already panel coordinates.

Result:

- a visually incorrect vertical rectangle refreshed first
- then the real content appeared in the expected place

We fixed this in our code by converting logical regions into panel-space regions before calling `display(region)`.

### 1.5 `Quality` and `Text` make coordinate errors easier to see

Because `quality` and `text` use slower and/or more multi-step waveforms, the incorrect region behavior was much more visible. `Fast` hides some of this because the update is quicker, but the coordinate bug was still real.

### 1.6 `Canvas` and `Direct` differ in where composition happens

`Direct`:

- draws straight into the main display framebuffer
- tends to repaint the whole logical block more aggressively

`Canvas`:

- composes a target region in an off-screen sprite first
- then pushes the sprite into the target area
- gives tighter control over what pixels actually change in the target region

This is the main reason `Fast + Canvas` behaves more like “changed glyph refresh” than `Fast + Direct`.

---

## 2. What The Driver Is Doing Internally

### 2.1 `_buf` stores target image content

`_buf` is the current desired panel content in packed grayscale form.

### 2.2 `_step_framebuf` stores refresh-progress state

This buffer does not just store pixels. It also stores refresh progression state for each pair of pixels, allowing the driver to continue multi-step waveform execution over time.

### 2.3 `_lut_2pixel` is a pre-expanded lookup table

The driver expands the LUT data into a form optimized for 2-pixel step processing and DMA output.

### 2.4 Region updates are queued and processed asynchronously

`Panel_EPD` pushes update requests into a queue. The background `task_update()` worker:

1. takes a region update
2. maps requested target pixels into waveform steps
3. advances `_step_framebuf`
4. generates scanline DMA buffers
5. writes lines to the EPD bus
6. repeats while unfinished steps remain

This means e-ink refreshes are not “instant blits”; they are staged waveform executions.

### 2.5 `fast` does not use the same erase-first behavior as `text/quality`

Key difference:

- `fast` / `fastest`: directly enqueue aggressive updates
- `text` / `quality`: often insert erase-like transition behavior before reaching the final value

That explains why `fast` looks more direct and better suited to small partial updates, while `text` and `quality` feel more whole-block and cleanup-oriented.

---

## 3. Production Recommendation

### 3.1 Stable full-page refresh baseline

Keep:

- full-page stable rendering path available at all times
- `Text` or `Quality` for cleanup/full-screen stable refreshes

### 3.2 Partial refresh baseline for inbox

Use:

- `Fast + Canvas`

And keep:

- logical-region to panel-region conversion before region submission

### 3.3 Do not use `Text` as the main message partial-refresh mode

`Text` is still useful for stable full refreshes, but it is not the best candidate for message-area incremental updates.

---

## 4. Plan For Message Partial Refresh

### Phase 1: Controlled inbox partial refresh

Target only the message page.

Behavior:

- new message arrives on the last page
- redraw only the newly affected text region using `Fast + Canvas`
- refresh page indicator separately if needed

Keep fallback:

- if anything is uncertain, fall back to the current stable full-frame submit path

### Phase 2: Separate top-bar region refresh

Split:

- message content region
- top status/page-indicator region

This allows page number changes to refresh independently from message-body changes.

### Phase 3: Periodic cleanup refresh

After some number of partial updates:

- perform a cleanup refresh using `Text` or `Quality`

Goal:

- reduce accumulated ghosting
- restore visual stability

---

## 5. Plan For “Wipe-Through” Page Transition

The requested effect is not traditional high-frame-rate animation.

On e-ink, the correct way to approximate it is:

### 5.1 Stripe-based progressive refresh

When page changes:

1. prepare the new page in canvas-backed strips
2. split the message viewport into a small number of vertical bands
3. refresh those bands left-to-right or right-to-left using `Fast + Canvas`

Recommended first version:

- 4 vertical strips
- each strip = 1/4 of the message viewport width

### 5.2 Final settle pass

After the strip sweep:

- perform one final refresh for the whole message viewport using the same target content

Goal:

- ensure the final page is visually consistent
- remove strip-boundary artifacts

### 5.3 Cleanup policy

After repeated animated transitions:

- trigger a full cleanup refresh using `Text` or `Quality`

This should be threshold-driven rather than done on every flip.

---

## 6. Why We Should Not Over-animate

Too many strips or too many rapid region updates will likely:

- increase ghosting
- queue too many EPD operations
- reduce perceived clarity
- look worse than a simpler controlled sweep

So the target should be:

- few steps
- strong directional cue
- fast settle into a clean final page

Not:

- LCD-style frame animation

---

## 7. Recommended Next Implementation Order

1. Apply `Fast + Canvas` only to real inbox message-region partial refresh
2. Keep top-bar refresh separate
3. Add cleanup threshold using a slower stable mode
4. After stable inbox partial refresh works, prototype 4-strip page-transition sweep
5. Tune visual effect only after stability is confirmed

---

## 8. Decision Summary

Lock in these decisions:

- production full refresh remains available
- inbox partial refresh baseline is `Fast + Canvas`
- panel-coordinate compensation stays in place for region submission
- `Text` is for stable full refresh, not primary message partial refresh
- page-transition “animation” should be implemented as stripe-based progressive EPD updates, not LCD-style frame animation
