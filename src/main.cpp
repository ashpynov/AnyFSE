
#include <windows.h>
#include <iostream>
#include "resource.h"
#include <tchar.h>
#include <commctrl.h>
#include <strsafe.h>
#include "Application.hpp"
#include "Service.hpp"
#include "Tools/Process.hpp"
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