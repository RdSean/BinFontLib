#pragma once

#include <stdint.h>

namespace papers3edc {

constexpr uint32_t kDefaultChargingPollIntervalSec = 2;
constexpr uint32_t kDefaultBatteryPollIntervalSec = 5;
constexpr uint32_t kMinAllowedPollIntervalSec = 2;
constexpr uint32_t kLegacyDefaultBatteryPollIntervalSec = 10;
constexpr uint32_t kLegacyObservedChargingPollIntervalSec = 4;
constexpr uint32_t kLegacyObservedBatteryPollIntervalSec = 0;

inline bool normalizePollIntervals(uint32_t& chargingSec, uint32_t& batterySec) {
    bool changed = false;

    if (chargingSec == kLegacyObservedChargingPollIntervalSec &&
        batterySec == kLegacyObservedBatteryPollIntervalSec) {
        chargingSec = kDefaultChargingPollIntervalSec;
        batterySec = kDefaultBatteryPollIntervalSec;
        return true;
    }

    if (chargingSec < kMinAllowedPollIntervalSec) {
        chargingSec = kDefaultChargingPollIntervalSec;
        changed = true;
    }

    if (batterySec < kMinAllowedPollIntervalSec) {
        batterySec = kDefaultBatteryPollIntervalSec;
        changed = true;
    }

    if (chargingSec == kDefaultChargingPollIntervalSec &&
        batterySec == kLegacyDefaultBatteryPollIntervalSec) {
        batterySec = kDefaultBatteryPollIntervalSec;
        changed = true;
    }

    return changed;
}

}  // namespace papers3edc
