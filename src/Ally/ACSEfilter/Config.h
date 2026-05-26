#pragma once

#include <windows.h>

namespace ACSEFilter::Config
{

    // ASUS USB vendor ID.
    inline constexpr USHORT kTargetVendorId = 0x0B05;

    inline constexpr USHORT kTargetProductIds[] = { 0x1ABE, 0x1B4C };

    inline constexpr DWORD kExpectedReadLength = 6;

    // Bytes returned to ASUS Optimization software when a matching HID read completes.
    inline constexpr BYTE kReplacementBytes[kExpectedReadLength] = { 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00 };

    inline constexpr BYTE kReplacementKeys[] =
    {
          0x38 // AC_PRESS
        , 0x93 // LIBRARY_PRESS
        , 0xA6 // CC_PRESS
        , 0xA7 // AC_HOLD
        , 0xA8 // AC_HOLD_RELEASE
    };

} // namespace ACSEFilter::config
