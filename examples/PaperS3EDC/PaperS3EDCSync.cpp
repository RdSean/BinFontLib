#include "PaperS3EDCSync.h"

#include <ArduinoJson.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

namespace papers3edc {
namespace {

struct RemoteTaskSeed {
    uint16_t id;
    const char* title;
    uint16_t points;
};

constexpr RemoteTaskSeed kRemoteTasks[] = {
    {101, "晨读二十分钟", 2},
    {102, "完成数学口算一页", 3},
    {103, "练字十五分钟", 2},
    {104, "整理书包和书桌", 1},
    {106, "睡前复盘今天收获", 2},
};

constexpr const char* kMessageBackendBaseUrl = "http://101.133.156.85:51007";
constexpr const char* kMessageDeviceEndpoint = "/api/device/messages";
constexpr const char* kMessageDeviceToken = "paper-s3-local-token";
constexpr const char* kMessageSource = "dingtalk";
constexpr uint8_t kMessageRequestLimit = kMaxMessages < 16 ? kMaxMessages : 16;
constexpr size_t kMessageJsonCapacity = 8192;

void copyText(char* dest, size_t capacity, const char* src) {
    if (!dest || capacity == 0) return;
    if (!src) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, capacity - 1);
    dest[capacity - 1] = '\0';
}

const TaskItem* findLocalTask(const AppState& state, uint16_t taskId) {
    for (size_t index = 0; index < state.taskCount; ++index) {
        if (state.tasks[index].id == taskId) {
            return &state.tasks[index];
        }
    }
    return nullptr;
}

int findMessageIndex(const AppState& state, const char* messageId) {
    for (size_t index = 0; index < state.messageCount; ++index) {
        if (strcmp(state.messages[index].message_id, messageId) == 0) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void copyBackendMessage(MessageItem& item, JsonObjectConst message) {
    char messageId[sizeof(item.message_id)] = {0};
    snprintf(messageId, sizeof(messageId), "%lu", message["id"] | 0UL);
    copyText(item.message_id, sizeof(item.message_id), messageId);
    copyText(item.title, sizeof(item.title), message["title"] | "");
    copyText(item.body, sizeof(item.body), message["body"] | "");
    copyText(item.sent_at, sizeof(item.sent_at), message["sent_at"] | "");
    copyText(item.source, sizeof(item.source), kMessageSource);
}

bool mergeMessage(AppState& state, const MessageItem& incoming) {
    const int existingIndex = findMessageIndex(state, incoming.message_id);
    if (existingIndex >= 0) {
        const bool wasRead = state.messages[existingIndex].read;
        state.messages[existingIndex] = incoming;
        state.messages[existingIndex].read = wasRead;
        return false;
    }

    if (state.messageCount == kMaxMessages) {
        for (size_t shift = kMaxMessages - 1; shift > 0; --shift) {
            state.messages[shift] = state.messages[shift - 1];
        }
        state.messages[0] = incoming;
        return true;
    }

    for (size_t shift = state.messageCount; shift > 0; --shift) {
        state.messages[shift] = state.messages[shift - 1];
    }
    state.messages[0] = incoming;
    ++state.messageCount;
    return true;
}

}  // namespace

bool PaperS3EDCSyncService::pullTodayTasks(AppState& state) {
    if (WiFi.status() != WL_CONNECTED) {
        state.syncState = SyncState::Offline;
        snprintf(state.statusLine, sizeof(state.statusLine), "%s", "当前离线");
        snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", "联网后拉取");
        return false;
    }

    state.syncState = SyncState::Pending;
    Serial.printf("[sync] pull begin current=%u\n", (unsigned)state.taskCount);

    TaskItem merged[kMaxTasks]{};
    size_t mergedCount = 0;
    for (size_t index = 0; index < (sizeof(kRemoteTasks) / sizeof(kRemoteTasks[0])) && mergedCount < kMaxTasks; ++index) {
        const RemoteTaskSeed& remote = kRemoteTasks[index];
        TaskItem task{};
        task.id = remote.id;
        task.points = remote.points;
        copyText(task.title, sizeof(task.title), remote.title);

        if (const TaskItem* local = findLocalTask(state, remote.id)) {
            task.status = isTaskDone(*local) ? TaskStatus::Done : TaskStatus::Pending;
            task.dirty = local->dirty;
        } else {
            task.status = TaskStatus::Pending;
            task.dirty = false;
        }
        merged[mergedCount++] = task;
        Serial.printf("[sync] pull task id=%u title=%s points=%u status=%u\n",
                      task.id,
                      task.title,
                      task.points,
                      static_cast<unsigned>(task.status));
    }

    for (size_t index = 0; index < mergedCount; ++index) {
        state.tasks[index] = merged[index];
    }
    for (size_t index = mergedCount; index < kMaxTasks; ++index) {
        state.tasks[index] = TaskItem{};
    }
    state.taskCount = mergedCount;
    state.selectedTaskIndex = 0;
    state.armedTaskIndex = -1;
    state.syncState = SyncState::Success;
    copyText(state.config.lastSyncLabel, sizeof(state.config.lastSyncLabel), "刚刚完成同步");
    snprintf(state.statusLine, sizeof(state.statusLine), "%s", "今日任务已更新");
    snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", "已拉取任务");
    return true;
}

bool PaperS3EDCSyncService::pullMessages(AppState& state, const char* deviceId) {
    if (WiFi.status() != WL_CONNECTED) {
        state.syncState = SyncState::Offline;
        snprintf(state.statusLine, sizeof(state.statusLine), "%s", "当前离线");
        snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", "联网后拉取消息");
        return false;
    }

    state.syncState = SyncState::Pending;
    const char* resolvedDeviceId = (deviceId && deviceId[0]) ? deviceId : "papers3-demo";
    const size_t freeSlots = (state.messageCount < kMaxMessages) ? (kMaxMessages - state.messageCount) : 0;
    const size_t requestLimit = std::min<size_t>(kMessageRequestLimit, std::max<size_t>(freeSlots, 1));
    if (requestLimit == 0) {
        state.syncState = SyncState::Success;
        snprintf(state.statusLine, sizeof(state.statusLine), "%s", "消息缓存已满");
        snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", "等待翻页或清空缓存");
        return false;
    }

    bool changed = false;
    String requestUrl = String(kMessageBackendBaseUrl) + kMessageDeviceEndpoint;
    requestUrl += "?device_id=";
    requestUrl += resolvedDeviceId;
    requestUrl += "&limit=";
    requestUrl += String(requestLimit);
    Serial.printf("[relay] GET %s\n", requestUrl.c_str());

    HTTPClient http;
    if (!http.begin(requestUrl)) {
        state.syncState = SyncState::Failed;
        snprintf(state.statusLine, sizeof(state.statusLine), "%s", "消息拉取失败");
        snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", "连接后端失败");
        return false;
    }
    http.addHeader("X-Device-Token", kMessageDeviceToken);
    const int statusCode = http.GET();
    if (statusCode != HTTP_CODE_OK) {
        http.end();
        state.syncState = SyncState::Failed;
        snprintf(state.statusLine, sizeof(state.statusLine), "%s", "接口返回异常");
        snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", "服务端未正常响应");
        return false;
    }

    const String response = http.getString();
    http.end();

    DynamicJsonDocument document(kMessageJsonCapacity);
    const DeserializationError error = deserializeJson(document, response);
    if (error) {
        state.syncState = SyncState::Failed;
        snprintf(state.statusLine, sizeof(state.statusLine), "%s", "消息拉取失败");
        snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", "消息数据解析失败");
        return false;
    }

    if (!document["messages"].is<JsonArray>() || !document["has_more"].is<bool>()) {
        state.syncState = SyncState::Failed;
        snprintf(state.statusLine, sizeof(state.statusLine), "%s", "接口返回异常");
        snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", "消息字段不完整");
        return false;
    }

    const bool hasMore = document["has_more"] | false;
    JsonArrayConst messages = document["messages"].as<JsonArrayConst>();
    for (JsonObjectConst remote : messages) {
        MessageItem item{};
        copyBackendMessage(item, remote);
        item.read = false;
        if (mergeMessage(state, item)) {
            changed = true;
        }
    }

    state.syncState = SyncState::Success;
    if (changed) {
        copyText(state.config.lastSyncLabel, sizeof(state.config.lastSyncLabel), "消息已刷新");
        snprintf(state.statusLine, sizeof(state.statusLine), "%s", "消息已更新");
        snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", hasMore ? "仍有更多未读消息" : "已完成本轮拉取");
    } else {
        snprintf(state.statusLine, sizeof(state.statusLine), "%s", "消息无更新");
        snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", hasMore ? "仍有未读消息待取" : "当前已经最新");
    }
    return changed;
}

bool PaperS3EDCSyncService::pushPendingEvents(AppState& state) {
    if (WiFi.status() != WL_CONNECTED) {
        state.syncState = SyncState::Offline;
        snprintf(state.statusLine, sizeof(state.statusLine), "%s", "当前离线");
        snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", "联网后同步");
        return false;
    }

    state.syncState = SyncState::Pending;
    Serial.printf("[sync] begin pending=%u\n", (unsigned)state.pendingEventCount);
    for (size_t index = 0; index < state.pendingEventCount; ++index) {
        const PendingEvent& event = state.pendingEvents[index];
        Serial.printf("[sync] event task=%u points=%u at=%u\n", event.taskId, event.points, event.completedAt);
    }

    state.pendingEventCount = 0;
    for (size_t index = 0; index < state.taskCount; ++index) {
        state.tasks[index].dirty = false;
    }

    state.syncState = SyncState::Success;
    copyText(state.config.lastSyncLabel, sizeof(state.config.lastSyncLabel), "刚刚完成上报");
    snprintf(state.statusLine, sizeof(state.statusLine), "%s", "模拟同步完成");
    snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", "等待真实接口");
    return true;
}

}  // namespace papers3edc
