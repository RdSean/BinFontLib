# PaperS3 Power-Off Splash Design

**Goal:** 在 PaperS3 EDC 固件里为硬件电源键长按关机增加一个明确的关机屏保，避免用户分不清设备是否已经关机。

**Chosen Approach:** 监听 `M5.BtnPWR` 的长按事件，阈值设为 3 秒。触发后先以高质量全屏重绘方式显示两行固定文案，再调用 `M5.Power.powerOff()` 完成关机。

**Why this approach:**
- 现有代码里没有任何电源键处理逻辑，但 `M5Unified` 暴露了 `BtnPWR`，应用层可以拦截长按。
- 先绘制再关机比依赖系统默认黑屏或残留画面更明确。
- 3 秒阈值足够高，不容易误触。

**Rendering behavior:**
- 使用现有 `PaperS3EDCFontManager` 的 bin 字体渲染。
- 屏幕内容为两行，整体在屏幕中部居中。
- 采用高质量整屏刷新，确保关机前画面稳定可读。

**Text content:**
- 第一行：`压反应釜冷却至 40°C`
- 第二行：`气相回路压力归零`

**Non-goals:**
- 不新增 UI 按钮关机入口。
- 不修改现有页面状态或持久化结构。
- 不改变普通电源管理/轮询逻辑。
