#pragma once

#include "PaperS3EDCModel.h"

namespace papers3edc {

class PaperS3EDCSyncService {
public:
    bool pullTodayTasks(AppState& state);
    bool pullMessages(AppState& state, const char* deviceId);
    bool pushPendingEvents(AppState& state);
};

}  // namespace papers3edc
