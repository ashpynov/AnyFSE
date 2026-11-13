#pragma once

#include <windows.h>

namespace AnyFSE::Tools::ShellWindow
{
    // Undocumented ImmersiveShell API. Requires COM initialized on the calling thread.
    HRESULT SwitchTo(HWND window);
}

namespace ShellWindow = AnyFSE::Tools::ShellWindow;
