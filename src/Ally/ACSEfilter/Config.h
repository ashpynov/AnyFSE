#pragma once

#include <windows.h>

namespace ACSEFilter::Config
{

    inline constexpr DWORD kAtkWmiIoControlCode = 0x0022240C;
    inline constexpr DWORD kAtkDevsMethod = 0x53564544;
    inline constexpr DWORD kAtkHidEventDeviceId = 0x00100021;

    inline constexpr BYTE kSuppressedButtonCodes[] =
    {
          0x38 // AC_PRESS
        , 0x93 // LIBRARY_PRESS
        , 0xA6 // CC_PRESS
        , 0xA7 // AC_HOLD
        , 0xA8 // AC_HOLD_RELEASE
    };

} // namespace ACSEFilter::Config
