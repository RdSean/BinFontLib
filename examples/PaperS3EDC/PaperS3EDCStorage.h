#pragma once

#include <FS.h>

#include "PaperS3EDCModel.h"

namespace papers3edc {

class PaperS3EDCStorage {
public:
    explicit PaperS3EDCStorage(fs::FS* filesystem = nullptr) : _filesystem(filesystem) {}

    void setFilesystem(fs::FS* filesystem) { _filesystem = filesystem; }
    bool isReady() const { return _filesystem != nullptr; }

    bool loadConfig(DeviceConfig& config);
    bool saveConfig(const DeviceConfig& config);
    bool loadState(AppState& state);
    bool saveState(const AppState& state);

private:
    fs::FS* _filesystem = nullptr;
};

}  // namespace papers3edc
