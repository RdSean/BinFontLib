# PaperS3 Multi-Font Diagnostics Design

**Context**

The device already has:

- a main UI font path that uses `霞鹜文楷32.bin`
- a settings-page-only diagnostic path that can load and render `霞鹜文楷28.bin`

The new requirement is to extend this diagnostic flow so multiple candidate fonts can be pushed into LittleFS and rendered on-device for side-by-side verification, without changing the main UI font.

Candidate fonts for this round:

- `霞鹜文楷28.bin`
- `LXGWWenKaiLite-for-edc-24.bin`
- `LXGWWenKai-24.bin`

**Goal**

Add a settings-page diagnostics section that can verify multiple candidate bin fonts from LittleFS, showing whether each font loads successfully and whether it can render a short Chinese sample correctly.

**Scope**

In scope:

- copy both new `24.bin` files into `data/`
- upload them into LittleFS
- extend diagnostics from one preview font to multiple preview fonts
- display load status, source path, font height, and a sample snippet per font

Out of scope:

- changing the main UI font
- adding runtime font switching
- adding a dedicated diagnostics page

**Approach Options**

1. Extend the current settings-page diagnostics into a compact multi-font list.

Pros:

- smallest change set
- keeps all font validation in one place
- does not add navigation

Cons:

- vertical space is tight and needs compact layout

2. Add a separate font test page.

Pros:

- more space for samples

Cons:

- adds navigation and page state just for temporary verification

3. Replace the main UI font one by one.

Pros:

- strongest real-world test

Cons:

- risky and unnecessary for diagnosing file validity

**Recommendation**

Use option 1. Keep the current settings page as the diagnostics surface, but compress the diagnostics into multiple small sections with one or two lines of metadata plus a short rendered sample for each font.

**Rendering Model**

The main UI continues to use the current primary font runtime.

Diagnostics should use isolated preview runtimes so each test font is loaded independently. A failure in one preview font must not affect:

- the main UI font
- other preview fonts

**What to Show**

For each candidate font:

- a label, for example `28`, `Lite24`, `WenKai24`
- load status
- font height
- storage/path
- a short sample such as:
  - `今天要完成的事`
  - `钉钉消息：你好，凌日奇观`

If a font fails to load:

- show failure state only
- do not attempt to render the sample

**Verification Standard**

This round should let us answer three questions for each font:

1. Does the file exist in LittleFS?
2. Can the device load it as a valid bin font?
3. Can it render a short Chinese sample without corruption?

If all three are true, the file is usable for later UI experiments.
