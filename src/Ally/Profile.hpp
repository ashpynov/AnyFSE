#pragma once

#include <windows.h>

namespace Ally::Profile
{
    enum class Mode : DWORD
    {
        Auto = 0,
        Gamepad = 1,
        Desktop = 2,
        Pause = 3
    };

    void SetAuto();         // Win+Alt+Ctrl+F8
    void SetGamepad();      // Win+Alt+Ctrl+F9
    void SetDesktop();      // Win+Alt+Ctrl+Shift+F9
    void SetToggle();       // Win+Alt+Ctrl+F10
    void SetNext();         // Win+Alt+Ctrl+Shift+F10
    void SetMode(Mode mode);
}
