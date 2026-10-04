#pragma once

#include <Geode/Geode.hpp>

namespace rickgdps::music::amoled {

    // AMOLED idle dim: while the Music Studio popup is open, the screen fades
    // to near-black after a configurable idle time (default 15s) - like an
    // always-on display with the current track. Any touch or key press wakes
    // it. Configured by the audio-amoled-* settings.
    void tick(float dt);
    void notifyActivity();
    void reset();
    bool isDimmed();

} // namespace rickgdps::music::amoled
