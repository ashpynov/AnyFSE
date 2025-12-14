// Power efficiency helpers
#pragma once

#include <windows.h>

namespace AnyFSE::Tools {
    // Force low-power mode for current process and threads (enable/disable).
    // If `bEnable` is true, lower process priority and enable execution-speed throttling.
    // If `bEnable` is false, restore normal priority and disable throttling.
    void EnablePowerEfficencyMode(bool bEnable);
}
