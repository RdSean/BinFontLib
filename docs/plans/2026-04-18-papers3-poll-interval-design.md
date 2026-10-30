# PaperS3 Poll Interval Migration Design

**Goal:** Change the PaperS3 EDC device polling defaults to `2s` while charging and `5s` on battery, and automatically correct known legacy persisted values when loading local state.

## Architecture

The backend API remains unchanged. The device continues to control `/api/device/messages` polling locally through `pollIntervalChargingSec` and `pollIntervalBatterySec`.

The implementation will introduce a small poll-policy normalization helper with no hardware dependencies. Device boot will call this helper after loading persisted state so legacy values such as the observed `charging=4`, `battery=0` pair are migrated to the new target pair `charging=2`, `battery=5`. The old compiled default pair `charging=2`, `battery=10` will also be migrated to the new battery default.

## Scope

- Update device defaults in `AppState`
- Normalize loaded persisted poll intervals
- Persist corrected values back to storage
- Leave the backend endpoint and manual settings UI behavior unchanged

## Testing

Add a host-side pytest that:

- Verifies the device defaults are `2` and `5`
- Compiles a small C++ harness against the poll-policy helper
- Asserts `charging=4, battery=0` migrates to `charging=2, battery=5`
- Asserts `charging=2, battery=10` migrates to `charging=2, battery=5`
- Asserts user-defined values such as `charging=6, battery=8` remain unchanged
