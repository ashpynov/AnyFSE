
#include <windows.h>
#include <iostream>
#include "resource.h"
#include <tchar.h>
#include <commctrl.h>
#include <strsafe.h>
#include "Application.hpp"
#include "Service.hpp"
#include "Process/Process.hpp"
#include "Logging/LogManager.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


// TODO Preventing multi launch

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    if (AnyFSE::Service::ShouldRunAsService(lpCmdLine))
    {
        return AnyFSE::Service::ServiceMain(hInstance, hPrevInstance, lpCmdLine);
    }
    else if (AnyFSE::Application::ShouldRunAsApplication(lpCmdLine))
    {
        return AnyFSE::Application::WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    }
    else
    {
        AnyFSE::Logging::LogManager::Initialize("AnyFSE", LogLevel::Trace, "C:\\Tools\\AnyFSE.log");
        Logger log = LogManager::GetLogger("Main");

        LPCTSTR className = _T("AnyFSE");

        HWND hAppWnd = FindWindow(className, NULL);

        if (hAppWnd)
        {
            log.Info("Application is executed already, exiting\n");
            PostMessage(hAppWnd, MainWindow::WM_TRAY, 0, WM_LBUTTONDBLCLK);
            return 0;
        }
        // if (!GamingExperience::ApiIsAvailable)
        // {
        //     log.Critical("Fullscreen Gaming API is not detected, exiting\n");
        //     InitCustomControls();
        //     TaskDialog(NULL, hInstance,
        //                L"Error",
        //                L"Gaming Fullscreen Experiense API is not detected",
        //                L"Fullscreen expiriense is not available on your version of windows.\n"
        //                L"It is supported since Windows 25H2 version for Handheld Devices",
        //                TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, NULL);
        //     return -1;
        // }

        if (!AnyFSE::Application::IsRunningAsAdministrator(true))
        {
            log.Critical("Application should be executed as Adminisrator, exiting\n");
            AnyFSE::Application::InitCustomControls();
            TaskDialog(NULL, hInstance,
                    L"Insufficient permissons",
                    L"Please run AnyFSE as Administrator",
                    L"Escalated privileges is required to monitor XBox application execution "
                    L"or instaling application as schedulled autorun task.\n\n",
                    TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, NULL);
            return -1;
        }

        int exitCode = 0;
        std::thread serviceThread = std::thread([&]()
                                    { AnyFSE::Service::ServiceMain(hInstance, hPrevInstance, NULL); });
        std::thread applicationThread = std::thread([&]()
                                    { exitCode = AnyFSE::Application::WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow); });

        applicationThread.join();
        AnyFSE::Service::ExitService();
        serviceThread.join();

        return exitCode;
    }
}