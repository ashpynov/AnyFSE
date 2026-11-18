#include "Ally/Haptic.hpp"
#include "Configuration/Config.hpp"
#include <xinput.h>

#pragma comment(lib, "xinput.lib")

namespace Ally::Haptic
{
    void Feedback(Pulse pulse)
    {
        if (!Config::AllyControlModeHaptic)
            return;

        constexpr WORD motorSpeed = MAXWORD * 3 / 4;
        constexpr DWORD pauseMs = 50;
        struct Pattern
        {
            XINPUT_VIBRATION vibration;
            int pulses;
            DWORD durationMs;
        };

        Pattern pattern;
        switch (pulse)
        {
        case Pulse::Short:  pattern = {{motorSpeed, motorSpeed}, 1, 50}; break;
        case Pulse::Single: pattern = {{motorSpeed, motorSpeed}, 1, 150}; break;
        case Pulse::Double: pattern = {{motorSpeed, motorSpeed}, 2, 100}; break;
        case Pulse::Triple: pattern = {{motorSpeed, motorSpeed}, 3, 50}; break;
        default:
            return;
        }

        XINPUT_VIBRATION stopped{};
        for (DWORD index = 0; index < XUSER_MAX_COUNT; ++index)
        {
            XINPUT_STATE state{};
            if (XInputGetState(index, &state) != ERROR_SUCCESS)
                continue;

            for (int pulse = 0; pulse < pattern.pulses; ++pulse)
            {
                if (pulse > 0)
                    Sleep(pauseMs);
                if (XInputSetState(index, &pattern.vibration) != ERROR_SUCCESS)
                    return;
                Sleep(pattern.durationMs);
                if (XInputSetState(index, &stopped) != ERROR_SUCCESS)
                    return;
            }
            return;
        }
    }
}
