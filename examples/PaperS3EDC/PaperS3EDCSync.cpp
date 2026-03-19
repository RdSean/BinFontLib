#include "PaperS3EDCSync.h"

#include <Arduino.h>
#include <WiFi.h>

namespace papers3edc {

bool PaperS3EDCSyncService::pushPendingEvents(AppState& state) {
    if (WiFi.status() != WL_CONNECTED) {
        state.syncState = SyncState::Offline;
        snprintf(state.statusLine, sizeof(state.statusLine), "%s", "当前离线");
        snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", "待连接网络后再同步");
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
    snprintf(state.statusLine, sizeof(state.statusLine), "%s", "已完成模拟同步");
    snprintf(state.statusDetail, sizeof(state.statusDetail), "%s", "后续接入真实云端接口");
    return true;
}

}  // namespace papers3edc
