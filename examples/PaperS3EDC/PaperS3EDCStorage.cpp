#include "PaperS3EDCStorage.h"

#include <Arduino.h>

namespace papers3edc {
namespace {
constexpr const char* kConfigPath = "/edc_config.txt";
constexpr const char* kStatePath = "/edc_state.txt";

void copyText(char* dest, size_t capacity, const char* src) {
    if (!dest || capacity == 0) return;
    if (!src) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, capacity - 1);
    dest[capacity - 1] = '\0';
}

String escapeText(const char* text) {
    String value = text ? text : "";
    value.replace("\\", "\\\\");
    value.replace("|", "\\p");
    value.replace("\n", " ");
    return value;
}

void unescapeText(char* dest, size_t capacity, String value) {
    value.replace("\\p", "|");
    value.replace("\\\\", "\\");
    copyText(dest, capacity, value.c_str());
}

String fieldAt(const String& line, int index) {
    int start = 0;
    int found = 0;
    while (found < index) {
        start = line.indexOf('|', start);
        if (start < 0) return String();
        ++start;
        ++found;
    }
    int end = line.indexOf('|', start);
    if (end < 0) end = line.length();
    return line.substring(start, end);
}

}  // namespace

bool PaperS3EDCStorage::loadConfig(DeviceConfig& config) {
    if (!_filesystem || !_filesystem->exists(kConfigPath)) {
        return false;
    }
    File file = _filesystem->open(kConfigPath, FILE_READ);
    if (!file) return false;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.startsWith("ssid=")) {
            copyText(config.ssid, sizeof(config.ssid), line.substring(5).c_str());
        } else if (line.startsWith("password=")) {
            copyText(config.password, sizeof(config.password), line.substring(9).c_str());
        } else if (line.startsWith("wifiConfigured=")) {
            config.wifiConfigured = line.substring(15).toInt() == 1;
        } else if (line.startsWith("lastSync=")) {
            copyText(config.lastSyncLabel, sizeof(config.lastSyncLabel), line.substring(9).c_str());
        }
    }
    file.close();
    return true;
}

bool PaperS3EDCStorage::saveConfig(const DeviceConfig& config) {
    if (!_filesystem) return false;
    File file = _filesystem->open(kConfigPath, FILE_WRITE);
    if (!file) return false;
    file.println("EDC_CONFIG_V1");
    file.printf("ssid=%s\n", config.ssid);
    file.printf("password=%s\n", config.password);
    file.printf("wifiConfigured=%d\n", config.wifiConfigured ? 1 : 0);
    file.printf("lastSync=%s\n", config.lastSyncLabel);
    file.close();
    return true;
}

bool PaperS3EDCStorage::loadState(AppState& state) {
    if (!_filesystem || !_filesystem->exists(kStatePath)) {
        return false;
    }
    File file = _filesystem->open(kStatePath, FILE_READ);
    if (!file) return false;

    state.taskCount = 0;
    state.pendingEventCount = 0;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (!line.length()) continue;
        if (line.startsWith("totalPoints=")) {
            state.summary.totalPoints = line.substring(12).toInt();
        } else if (line.startsWith("streakDays=")) {
            state.summary.streakDays = line.substring(11).toInt();
        } else if (line.startsWith("task|")) {
            if (state.taskCount >= kMaxTasks) continue;
            TaskItem& task = state.tasks[state.taskCount++];
            task.id = fieldAt(line, 1).toInt();
            task.points = fieldAt(line, 2).toInt();
            task.status = static_cast<TaskStatus>(fieldAt(line, 3).toInt());
            task.dirty = fieldAt(line, 4).toInt() == 1;
            unescapeText(task.title, sizeof(task.title), fieldAt(line, 5));
        } else if (line.startsWith("pending|")) {
            if (state.pendingEventCount >= kMaxPendingEvents) continue;
            PendingEvent& event = state.pendingEvents[state.pendingEventCount++];
            event.taskId = fieldAt(line, 1).toInt();
            event.points = fieldAt(line, 2).toInt();
            event.completedAt = fieldAt(line, 3).toInt();
        } else if (line.startsWith("contrib=")) {
            String payload = line.substring(8);
            int start = 0;
            for (size_t index = 0; index < kContributionCellCount; ++index) {
                int comma = payload.indexOf(',', start);
                String field = (comma < 0) ? payload.substring(start) : payload.substring(start, comma);
                state.contribution[index].level = field.toInt();
                if (comma < 0) break;
                start = comma + 1;
            }
        }
    }
    file.close();
    return state.taskCount > 0;
}

bool PaperS3EDCStorage::saveState(const AppState& state) {
    if (!_filesystem) return false;
    File file = _filesystem->open(kStatePath, FILE_WRITE);
    if (!file) return false;
    file.println("EDC_STATE_V1");
    file.printf("totalPoints=%u\n", state.summary.totalPoints);
    file.printf("streakDays=%u\n", state.summary.streakDays);
    for (size_t index = 0; index < state.taskCount; ++index) {
        const TaskItem& task = state.tasks[index];
        file.printf("task|%u|%u|%u|%u|%s\n",
                    task.id,
                    task.points,
                    static_cast<unsigned>(task.status),
                    task.dirty ? 1 : 0,
                    escapeText(task.title).c_str());
    }
    for (size_t index = 0; index < state.pendingEventCount; ++index) {
        const PendingEvent& event = state.pendingEvents[index];
        file.printf("pending|%u|%u|%u\n", event.taskId, event.points, event.completedAt);
    }
    file.print("contrib=");
    for (size_t index = 0; index < kContributionCellCount; ++index) {
        if (index) file.print(',');
        file.print(state.contribution[index].level);
    }
    file.println();
    file.close();
    return true;
}

}  // namespace papers3edc
