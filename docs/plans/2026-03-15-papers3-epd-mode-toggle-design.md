# PaperS3 EPD Mode Toggle Design

**Date:** 2026-03-15
**Repo:** `BinFontLib`
**Branch:** `papers3-binfont-demo`

## Goal

Add a second header button to the PaperS3 demo so the user can switch the active EPD refresh mode at runtime and compare visual behavior and timing for `fast`, `text`, `quality`, and `fastest`.

## Approved UI

- Keep the existing header button for benchmark strategy: `Bench:Direct` / `Bench:Stripe`
- Add a new adjacent header button for EPD mode: `EPD:Fast`, `EPD:Text`, `EPD:Quality`, `EPD:Fastest`
- Both buttons are always visible and touchable from every page

## Behavior

- Tapping the benchmark button cycles `Direct -> Stripe -> Direct`
- Tapping the EPD button cycles `Fast -> Text -> Quality -> Fastest -> Fast`
- `Preview` and `Glyphs` pages re-render immediately using the selected EPD mode
- `Benchmark` page re-runs the benchmark suite after either mode changes
- Serial logs and benchmark summary should include the active EPD mode label

## Scope

### In scope
- Header UI layout update for two independent controls
- Runtime state for selected EPD mode
- Mapping from UI selection to `M5GFX` `epd_mode_t`
- Use selected EPD mode in normal page refreshes and benchmark reporting

### Out of scope
- No new popup menu system
- No long-press interaction
- No attempt to measure guaranteed physical refresh completion at the panel driver level

## Implementation Notes

- The current demo already has a benchmark strategy button in the header; the new EPD button should reuse the same button drawing style.
- The selected EPD mode should be applied through a small helper that returns both the `epd_mode_t` value and the display label.
- The benchmark logs should print both benchmark strategy and EPD mode so captured serial output remains self-describing.

## Validation

- Build the `papers3-demo` PlatformIO environment
- Flash to the connected PaperS3 on `/dev/cu.usbmodem2101`
- Verify the device boots, the font still loads, and the benchmark runs
- Manually verify that tapping the new EPD button changes the header label and refresh behavior
