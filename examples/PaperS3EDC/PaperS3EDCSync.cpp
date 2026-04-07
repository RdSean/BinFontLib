#include "PaperS3EDCSync.h"

#include <Arduino.h>
#include <WiFi.h>

namespace papers3edc {
namespace {

struct RemoteTaskSeed {
    uint16_t id;
    const char* title;
    uint16_t points;
};

struct RemoteMessageSeed {
    const char* cursor;
    const char* messageId;
    const char* title;
    const char* body;
    const char* sentAt;
    const char* source;
};

constexpr RemoteTaskSeed kRemoteTasks[] = {
    {101, "晨读二十分钟", 2},
    {102, "完成数学口算一页", 3},
    {103, "练字十五分钟", 2},
    {104, "整理书包和书桌", 1},
    {106, "睡前复盘今天收获", 2},
};

constexpr RemoteMessageSeed kRelayMessages[] = {
    {"relay-cursor-001", "relay_2001", "钉钉班级通知", "语文练习请在今晚 20:00 前完成，明早统一检查。", "10:08", "relay://dingtalk"},
    {"relay-cursor-002", "relay_2002", "家长提醒", "雨天路滑，放学后直接到东门等候，不要自己绕路。", "12:16", "relay://family"},
    {"relay-cursor-003", "relay_2003", "老师留言", "明天带水彩笔和剪刀，下午美术课会统一使用。", "16:42", "relay://teacher"},
};

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
    Serial.printf("[relay] GET /messages?device_id=%s&cursor=%s\n",
                  deviceId ? deviceId : "papers3-edc",
                  state.lastMessageCursor[0] ? state.lastMessageCursor : "(none)");

    size_t startIndex = 0;
    if (state.lastMessageCursor[0]) {
        for (size_t index = 0; index < (sizeof(kRelayMessages) / sizeof(kRelayMessages[0])); ++index) {
            if (strcmp(kRelayMessages[index].cursor, state.lastMessageCursor) == 0) {
                startIndex = index + 1;
                break;
            }
        }
    }

    bool changed = false;
    for (size_t index = startIndex; index < (sizeof(kRelayMessages) / sizeof(kRelayMessages[0])); ++index) {
        const RemoteMessageSeed& remote = kRelayMessages[index];
        MessageItem item{};
        copyText(item.message_id, sizeof(item.message_id), remote.messageId);
        copyText(item.title, sizeof(item.title), remote.title);
        copyText(item.body, sizeof(item.body), remote.body);
        copyText(item.sent_at, sizeof(item.sent_at), remote.sentAt);
        copyText(item.source, sizeof(item.source), remote.source);
        item.read = false;

        const int existing = findMessageIndex(state, remote.messageId);
        if (existing >= 0) {
            state.messages[existing] = item;
        } else {
            if (state.messageCount == kMaxMessages) {
                for (size_t shift = state.messageCount - 1; shift > 0; --shift) {
                    state.messages[shift] = state.messages[shift - 1];
                }
                state.messages[0] = item;
            } else {
                for (size_t shift = state.messageCount; shift > 0; --shift) {
                    state.messages[shift] = state.messages[shift - 1];
                }
                state.messages[0] = item;
                ++state.messageCount;
            }
        }

        copyText(state.lastMessageCursor, sizeof(state.lastMessageCursor), remote.cursor);
        changed = true;
    }

    state.syncState = SyncState::Success;
    if (changed) {
        copyText(state.config.lastSyncLabel, sizeof(state.config.lastSyncLabel), "消息已刷新");
        snprintf(state.statusLine, sizeof(state.statusLine), "%s", "消息已更新");
        snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", "relay 返回新消息");
    } else {
        snprintf(state.statusLine, sizeof(state.statusLine), "%s", "消息无更新");
        snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", "cursor 已是最新");
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
