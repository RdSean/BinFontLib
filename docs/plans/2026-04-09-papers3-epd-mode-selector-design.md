# PaperS3 EPD Mode Selector Design

**Context**

The PaperS3 inbox renderer currently hardcodes `epd_mode_t::epd_text` for every frame submission. That gives a stable text-oriented baseline, but it prevents on-device comparison of faster e-ink modes such as `Fast` or `Fastest`.

The settings page already exposes tap-driven device options, so it is the lowest-friction place to expose EPD refresh mode selection for live testing.

**Goal**

Add an on-device EPD refresh mode selector in the settings page, persist the selected mode in LittleFS config, and make the renderer apply the selected hardware mode on every display submission.

**Scope**

In scope:

- define a persisted app config field for EPD mode
- show the current EPD mode in the settings list
- support tap-based cycling between `Text`, `Fast`, `Fastest`, and `Quality`
- make renderer use the configured mode for frame submission

Out of scope:

- redesigning the settings page layout
- automatic per-page mode switching
- restoring hardware partial refresh

**Approach Options**

1. Add the selector to the existing settings page.

Pros:

- minimal UI change
- no extra page flow
- easiest to compare modes live

Cons:

- one more row in the settings list

2. Add a separate diagnostics page for EPD modes.

Pros:

- more room for explanation

Cons:

- slower interaction
- unnecessary navigation overhead for a simple toggle

**Recommendation**

Use option 1.

The settings page already uses left/right tap semantics to adjust values, which matches EPD mode cycling naturally.

**Design**

- Add `EpdRefreshMode` to app model with values `Text`, `Fast`, `Fastest`, `Quality`
- Store selected mode in `DeviceConfig`
- Persist mode in `/edc_config.txt`
- Increase settings row count by one and add a row:
  - `刷新模式  Text`
  - `刷新模式  Fast`
  - `刷新模式  Fastest`
  - `刷新模式  Quality`
- In settings page interaction:
  - tap left half of the row: previous mode
  - tap right half of the row: next mode
- Renderer `applyEpdMode()` maps config to `epd_mode_t`

**Success Criteria**

- user can switch EPD mode from device settings
- selected mode survives reboot
- renderer no longer hardcodes `epd_text`
- firmware builds and flashes successfully
