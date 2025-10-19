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
        AnyFSE::Logging::LogManager::Initialize("AnyFSE");
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