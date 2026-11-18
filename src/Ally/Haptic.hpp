#pragma once

#include <windows.h>

namespace Ally::Haptic
{
    enum class Pulse : DWORD
    {
        Short = 0,
        Single = 1,
        Double = 2,
        Triple = 3
    };

    void Feedback(Pulse pulse);
}
