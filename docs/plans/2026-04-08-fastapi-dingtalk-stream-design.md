# FastAPI 钉钉 Stream 服务端设计文档

**Date:** 2026-04-08
**Repo:** `BinFontLib`
**Branch:** `papers3-binfont-demo`

## 目标

为 PaperS3 EDC 消息功能补齐一个单家庭、单设备、无账号体系的服务端。服务端使用 FastAPI 作为设备读取接口，并使用钉钉官方 Stream 模式接收消息事件，在无公网 IP 的前提下完成“钉钉消息 -> 本地服务 -> PaperS3 轮询读取”的闭环。

## 当前约束

- 只服务 1 个家庭
- 只服务 1 台设备
- 不做用户注册、登录、账号系统
- 设备端只读，不回复消息
- 当前没有公网 IP
- 已确认钉钉支持 Stream 模式
- 希望使用 FastAPI

## 为什么选择 Stream 模式

- 不需要公网回调地址
- 本地开发环境可直接连接钉钉
- 更适合当前“单机服务 + 本地局域网设备”的开发阶段
- 避免为钉钉回调单独引入外网隧道或云主机

## 总体架构

服务端由三个部分组成：

### 1. FastAPI Web 服务

负责：

- 给 PaperS3 提供只读消息拉取接口
- 提供健康检查和可选调试接口

### 2. DingTalk Stream Worker

负责：

- 使用钉钉官方 Python Stream SDK 与钉钉建立长连接
- 接收钉钉事件
- 提取可显示消息内容
- 做消息归一化、去重与入库

### 3. SQLite 数据库

负责：

- 存储消息记录
- 提供设备轮询游标基础
- 保留原始事件以便排查问题

## 进程模型

采用两个独立进程：

- `FastAPI` 进程
- `Stream Worker` 进程

原因：

- Web 服务和长连接消费职责不同
- 更易于单独重启和排障
- 比“单进程同时跑 HTTP 和 Stream”更稳
- 对 MVP 足够简单

## 数据流

1. 钉钉通过 Stream 模式将事件送到本地 worker
2. Worker 校验并解析事件
3. Worker 将可展示消息归一化后写入 SQLite
4. PaperS3 周期性请求 FastAPI 的消息接口
5. FastAPI 按游标返回增量消息
6. 设备更新本地缓存与游标

## 数据模型

第一阶段只需要一张消息主表即可。

### `messages`

- `id`：自增主键，作为设备轮询游标
- `source_event_id`：钉钉事件唯一 ID，用于去重
- `source_type`：固定先使用 `dingtalk`
- `title`
- `body`
- `sent_at`
- `raw_payload`
- `created_at`

## 设备读取接口

### `GET /api/device/messages`

参数：

- `device_id`
- `since_id`
- `limit`

返回：

- `messages`
- `next_cursor`
- `has_more`
- `server_time`

### 设备鉴权

不做登录，但保留最小设备鉴权：

- 使用固定 `X-Device-Token`
- 服务端从环境变量读取 token
- token 校验失败直接返回 `401`

## 钉钉接入边界

### Worker 负责

- 调用钉钉 Stream SDK
- 订阅消息事件
- 将钉钉原始事件转换为统一消息格式
- 使用 `source_event_id` 去重

### FastAPI 不负责

- 不直接处理钉钉 Stream 连接
- 不承担钉钉 SDK 生命周期

这样可以避免 Web 生命周期和 Stream 生命周期耦合。

## 归一化策略

钉钉原始事件先转成设备端可消费的统一结构：

- `title`
- `body`
- `sent_at`
- `source_event_id`
- `source_type`

第一阶段只保留文本内容，不支持：

- 图片
- 文件附件
- 富文本复杂样式
- 回复动作

## 游标策略

设备读取使用 `since_id`。

- 设备发送最近已处理的最大 `id`
- 服务端查询 `id > since_id`
- 按 `id asc` 返回最多 `limit` 条
- `next_cursor` 返回最后一条消息的 `id`

选择自增 `id` 而不是时间戳的原因：

- 顺序稳定
- 去重清晰
- 设备实现简单

## 存储与部署

### 数据库

- 第一阶段使用 SQLite
- 数据文件落地在本地目录
- 足够支持单设备单家庭场景

### 部署

开发和初期运行可直接在一台本机或家庭服务器上部署：

- 一个 FastAPI 进程
- 一个 Stream Worker 进程
- 一个 SQLite 文件

## 配置项

建议环境变量：

- `DEVICE_TOKEN`
- `DINGTALK_CLIENT_ID`
- `DINGTALK_CLIENT_SECRET`
- `DATABASE_URL`

## 失败处理

- Stream 重连失败：记录日志并重试
- 重复事件：基于 `source_event_id` 忽略
- 入库失败：记录错误，避免进程静默失败
- 设备 token 错误：直接 `401`
- 设备读取失败：不影响已入库消息

## 第一阶段不做的内容

- 多设备支持
- 多家庭隔离
- Web 管理后台
- 用户登录
- 消息回复
- 已读回传钉钉
- 云数据库

## 验收标准

- 在无公网 IP 环境下，Stream Worker 能接收到钉钉事件
- FastAPI 能返回增量消息给设备
- SQLite 中能看到归一化后的消息记录
- 重复事件不会重复入库
- 设备 token 校验生效
- 服务端重启后历史消息仍保留
