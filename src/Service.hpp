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
