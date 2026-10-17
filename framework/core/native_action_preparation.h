#pragma once
#include "session.h"

namespace SCNativeAction {
enum class Preparation { Waiting, Ready, Cancelled };

class GroundSettle {
    SCSession::QuietGate quiet;
    uint32_t started = 0;

public:
    void begin(uint32_t now) {
        started = now;
        quiet.clear();
    }

    Preparation update(bool groundedIdle, bool focus, uint32_t now) {
        if (!focus || uint32_t(now - started) >= 1000) {
            quiet.clear();
            return Preparation::Cancelled;
        }
        return quiet.update(groundedIdle, now, 150) ? Preparation::Ready : Preparation::Waiting;
    }
};
} // namespace SCNativeAction
