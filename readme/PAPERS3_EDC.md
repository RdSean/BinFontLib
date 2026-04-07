# PaperS3 EDC MVP

## Goal

Build a local-first child-focused EDC task device on M5Stack PaperS3. The current MVP focuses on a task-first daily screen, completion confirmation, progress summary, a read-only message inbox, device settings, Wi‑Fi setup, and a sync-ready local data model.

## Files

- App entry: `examples/PaperS3EDC/PaperS3EDC.cpp`
- App controller: `examples/PaperS3EDC/PaperS3EDCApp.cpp`
- UI renderer: `examples/PaperS3EDC/PaperS3EDCRenderer.cpp`
- Font manager: `examples/PaperS3EDC/PaperS3EDCFontManager.cpp`
- Local storage: `examples/PaperS3EDC/PaperS3EDCStorage.cpp`
- Sync boundary: `examples/PaperS3EDC/PaperS3EDCSync.cpp`
- Build config: `platformio.ini`
- Filesystem asset: `data/霞鹜文楷32.bin`

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
- `消息`：消息收件箱列表
- `消息详情`：查看单条消息并滚动正文
- `配置`：Wi‑Fi、手动同步、返回今日
- `Wi‑Fi 列表`：扫描并选择网络
- `密码输入`：设备端矩形键盘输入

## Local-first behavior

- Fake tasks are seeded on first boot
- If `LittleFS` contains saved state, task state and points are restored
- Completed tasks write pending sync events locally
- Manual sync first pushes pending completion events over serial, then pulls a fake "today tasks" snapshot
- Pull sync keeps local completed tasks as the winning state so offline completion is not rolled back by a later refresh

## Stored files

The current MVP stores simple text files in `LittleFS`:

- `/edc_config.txt`
- `/edc_state.txt`

## Current limitations

- No real cloud API yet
- No mini program implementation yet
- Wi‑Fi keyboard is functional but still minimal
- Message inbox and contribution view are MVP versions intended for later visual polish
- Local storage is still a lightweight text format; it has not been upgraded to the JSON layout described in the March 20 implementation plan
