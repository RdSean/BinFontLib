# PaperS3 Unread Message Queue Design

**Goal**

把设备消息同步从“时间游标增量拉取”改成“未读消息队列”模型：设备默认只取未读消息，后端返回后立即标记为已读，设备端不再关心时间戳游标。

**Why**

当前基于 `since_ts` 的方案在设备清空、本地状态恢复、历史 backlog 跳过和局刷测试场景下都比较脆弱。设备端其实不需要知道“从什么时间之后开始取”，只需要拿到“现在还没读过的消息”。把这个状态收敛到后端，更适合当前产品形态。

## Data Model

`messages` 表新增：

- `is_read BOOLEAN NOT NULL DEFAULT FALSE`

新消息入库时默认 `FALSE`。

因为当前已经明确“不考虑兼容旧数据”，迁移策略可以直接走最简单路径：

- 开发环境重建数据库或执行一次性迁移
- 旧 `since_ts` / `next_cursor` 协议不再作为设备主路径依赖

## API Design

接口保持：

- `GET /api/device/messages`

参数改成：

- `device_id`
- `limit`
- `filter=unread|all`，默认 `unread`
- `debug=0|1`，默认 `0`

不再要求设备传 `since_ts`。

### Default Behavior

默认请求不带 `filter`，后端按 `unread` 处理：

- 只查询 `is_read = FALSE`
- 按 `sent_at ASC, id ASC` 返回最旧的 `limit` 条
- 返回成功后，在同一个请求内把这批记录批量更新为 `is_read = TRUE`

这样设备端看到的顺序天然是“旧消息在上，新消息在下”。

### All Mode

`filter=all` 时：

- 不按已读状态过滤
- 按 `sent_at DESC, id DESC` 返回最近 `limit` 条
- 不修改 `is_read`

这个模式主要给人工排查和开发测试使用，不是设备常规拉取路径。

### Debug Mode

`debug=1` 时：

- 不修改 `is_read`
- 不污染正常未读状态
- 可允许附加调试字段，但不是必须

推荐行为是：

- `debug=1` + 未显式指定 `filter` 时，也按 `all` 查询

这样调试不会影响生产语义。

## Device Behavior

设备端默认只发：

- `device_id`
- `limit`

不再发：

- `since_ts`
- `cursor`

设备端请求数要被本地缓存能力钳住：

- `limit <= kMaxMessages`
- 更稳妥的话，再单独定义一个更小的同步上限，例如 `kMessageRequestLimit = min(kMaxMessages, 16)`

这样后端不会一次返回超过设备可承受的消息数量，避免列表合并和分页测量在极端情况下失控。

本地仍保留消息缓存，但它只承担“显示和分页”职责，不再承担“同步边界”职责。

## Clear Behavior

清空消息缓存按钮简化为：

- 只清设备本地消息数组和分页状态
- 不再处理时间游标

如果需要“从后端重新拉未读消息”，下一轮正常拉取即可。

如果需要“连后端未读状态一起重置”，那应作为单独后台调试接口，不混到设备端常规 API。

## Rendering Implication

这个后端模型更适合当前局刷测试：

- 设备每轮只会拿到一小批真正未读消息
- 不再出现时间游标失配导致的大量历史消息回灌
- 最后一页追加局刷的输入会更稳定

## Testing

需要验证：

1. 新钉钉消息入库后 `is_read = FALSE`
2. 设备默认请求只拿未读消息，并且请求完成后这些消息被置为 `TRUE`
3. 再次默认请求时，不会重复拿到同一批消息
4. `filter=all` 返回最近 `limit` 条，但不改 `is_read`
5. `debug=1` 不污染已读状态
6. 设备请求 `limit` 被钳到安全范围，不会超过本地缓存能力
