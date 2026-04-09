# PaperS3 EDC MVP

## Goal

Build a local-first child-focused EDC task device on M5Stack PaperS3. The current MVP focuses on a task-first daily screen, completion confirmation, progress summary, a read-only message inbox, device settings, Wi‑Fi setup, and a sync-ready local data model.

The current PaperS3 device is read-only for messages: it only polls, caches, and displays relay-delivered content. Message creation, reply, and ack actions stay outside the device.

## Files

- App entry: `examples/PaperS3EDC/PaperS3EDC.cpp`
- App controller: `examples/PaperS3EDC/PaperS3EDCApp.cpp`
- UI renderer: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Font manager: `examples/PaperS3EDC/PaperS3EDCFontManager.cpp`
- Local storage: `examples/PaperS3EDC/PaperS3EDCStorage.cpp`
- Sync boundary: `examples/PaperS3EDC/PaperS3EDCSync.cpp`
- Build config: `platformio.ini`
- Filesystem asset: `data/LXGWWenKaiLite-for-edc-32.bin`

## Build

```bash
pio run -e papers3-edc
```

## Upload firmware

```bash
pio run -e papers3-edc -t upload --upload-port /dev/cu.usbmodem2101
```

## Upload font file to LittleFS

```bash
pio run -e papers3-edc -t uploadfs
```

## Monitor serial

```bash
pio device monitor -p /dev/cu.usbmodem2101 -b 115200
```

## Current MVP pages

- `今日`：任务优先的主屏
- `确认完成`：点击任务后进入确认
- `成长`：积分和贡献图摘要
- `消息`：无边框消息流，按轮次连续显示并整页滚动
- `配置`：Wi‑Fi、手动同步、返回今日
- `配置`页底部包含 `字体诊断` 区，本轮用于验证 `LXGWWenKaiLite-for-edc-28.bin` 是否能被真实加载和渲染
- `Wi‑Fi 列表`：扫描并选择网络
- `密码输入`：设备端矩形键盘输入

## Message relay flow

- Device side keeps a local message cache, cursor, and polling configuration in `LittleFS`
- `PaperS3EDCSyncService` exposes a message pull path that looks like `GET /messages?device_id=<id>&cursor=<cursor>`
- A relay service sits between DingTalk sources and the PaperS3 device
- The relay normalizes incoming DingTalk content into a compact read-only message payload
- The device merges new relay messages into local cache and updates the cursor after each successful poll

## Polling defaults

- Charging default: `2` seconds
- Battery default: `10` seconds
- The device chooses the active interval from current charging state
- Auto polling pauses while the `Wi‑Fi` password page is open
- Settings page allows adjusting both intervals directly on device

## Local-first behavior

- Fake tasks are seeded on first boot
- If `LittleFS` contains saved state, task state and points are restored
- If `LittleFS` contains saved messages, cursor, and polling settings, they are restored on boot
- Completed tasks write pending sync events locally
- Manual sync first pushes pending completion events, then pulls a fake "today tasks" snapshot and message relay payload
- Pull sync keeps local completed tasks as the winning state so offline completion is not rolled back by a later refresh
- Timed background polling refreshes the message inbox on the configured cadence
- Message inbox uses a single scrollable stream instead of per-message detail navigation
- The first message in a round gets a left-side marker; follow-up messages keep the same gutter without another marker
- Settings-page diagnostics can independently probe multiple candidate preview fonts from LittleFS

## Stored files

The current MVP stores simple text files in `LittleFS`:

- `/edc_config.txt`
- `/edc_state.txt`

## Current limitations

- Relay polling still uses a fake in-firmware payload instead of a real cloud API
- No mini program implementation yet
- Wi‑Fi keyboard is functional but still minimal
- Message inbox and contribution view are MVP versions intended for later visual polish
- Local storage is still a lightweight text format; it has not been upgraded to the JSON layout described in the March 20 implementation plan
