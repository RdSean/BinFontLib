#pragma once

#include "PaperS3EDCModel.h"

namespace papers3edc {

class PaperS3EDCSyncService {
public:
    bool pushPendingEvents(AppState& state);
};

}  // namespace papers3edc
