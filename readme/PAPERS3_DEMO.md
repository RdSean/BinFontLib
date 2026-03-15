# PaperS3 Demo

## Goal

Verify that `BinFontLib` can load and render the real `霞鹜文楷32.bin` font on M5Stack PaperS3 hardware, then show basic render timing on screen and over serial.

## Files

- Demo entry: `examples/PaperS3BinFontDemo/PaperS3BinFontDemo.cpp`
- App logic: `examples/PaperS3BinFontDemo/PaperS3DemoApp.cpp`
- Build config: `platformio.ini`
- Filesystem asset: `data/霞鹜文楷32.bin`

## Build

```bash
pio run -e papers3-demo
```

## Upload firmware

```bash
pio run -e papers3-demo -t upload
```

## Upload font file to LittleFS

```bash
pio run -e papers3-demo -t uploadfs
```

## Monitor serial

```bash
pio device monitor -p /dev/cu.usbmodem2101 -b 115200
```

## Expected behavior

- Startup logs show LittleFS / SD mount status
- The demo loads `霞鹜文楷32.bin`
- Device boots directly into the benchmark page
- Touch footer buttons switch among preview, glyph check, and benchmark pages
- Benchmark page prints average and max render time

## Fallback font search paths

- `/霞鹜文楷32.bin`
- `/fonts/霞鹜文楷32.bin`
- `/font/霞鹜文楷32.bin`

