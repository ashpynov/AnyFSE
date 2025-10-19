// AnyFSE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// AnyFSE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details. <https://www.gnu.org/licenses/>


#include <windows.h>
#include "Monitors/ETWMonitor.hpp"
#include "Manager/ManagerService.hpp"

namespace AnyFSE
{
    class Service
    {
        static HWND m_hwnd;
        static AnyFSE::Monitors::ETWMonitor etwMonitor;
        static AnyFSE::Manager::State::ManagerService managerState;
        static bool xboxIsDenied;
        static const std::wstring XBoxProcessName;

    public:
        static int WINAPI ServiceMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine);
        static BOOL ExitService();
        static BOOL ShouldRunAsService(LPSTR lpCmdLine);

    private:
        static BOOL LaunchAppInUserSession(DWORD sessionId);

        static void StartMonitoring();
        static void StopMonitoring();
        static void KillXbox();

        static LRESULT BackgroundWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        static void CreateBackgroundWindow();
        static void MonitorSessions();

    };
}
