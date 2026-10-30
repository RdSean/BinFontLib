# BinFontLib

Arduino library for rendering binary font files on embedded devices (ESP32).

Documentation:
- Main README: readme/README.md
- Quickstart: readme/QUICKSTART.md
- Usage: readme/USAGE.md
- API: readme/API.md

Example:
- examples/BinTestInteractive/
- examples/PaperS3BinFontDemo/
- examples/PaperS3EDC/

PlatformIO demo:
- `platformio.ini` contains ready-to-build `papers3-demo` and `papers3-edc` environments for M5Stack PaperS3
- `readme/PAPERS3_DEMO.md` explains the font benchmark demo
- `readme/PAPERS3_EDC.md` explains the EDC MVP build, flash, and local-first app flow

PaperS3 EDC summary:
- The `papers3-edc` example is a child-focused PaperS3 task device with a read-only message inbox
- The current message flow is relay-based: DingTalk-style sources feed a relay, and the device polls normalized messages by `device_id` and `cursor`
- Default message polling intervals are `2` seconds while charging and `10` seconds on battery
- Message reading is supported on device; message authoring and reply actions are intentionally out of scope for the current MVP

  # Next
  - I will change to PlatformIO for more posible.
