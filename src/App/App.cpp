// MIT License
//
// Copyright (c) 2025 Artem Shpynov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


#include <windows.h>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include "resource.h"
#include <tchar.h>
#include <commctrl.h>
#include <strsafe.h>

#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Process.hpp"
#include "Tools/Elevated.hpp"
#include "Tools/Notification.hpp"
#include "Tools/Registry.hpp"
#include "Tools/Localization.hpp"
#include "Tools/Unicode.hpp"

#include "App/App.hpp"
#include "App/CmdLine.hpp"
#include "App/Constants.hpp"
#include "App/GamingExperience.hpp"
#include "App/ExitFSE.hpp"
#include "App/MainWindow.hpp"
#include "App/Launchers.hpp"
#include "App/JumpList.hpp"
#include "Ally/Ally.hpp"
#include "Ally/Handlers.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#include "Tools/Minidump.hpp"

namespace AnyFSE::App
{
    static Logger log = LogManager::GetLogger("App");

    int CallLibrary(const WCHAR * library, HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
    {
        HMODULE hModuleDll = NULL;
        static MainFunc *Main = nullptr;

        int result = INT_MIN;
        do
        {
            hModuleDll = LoadLibrary(library);
            if (!hModuleDll)
            {
                break;
            }

            MainFunc Main = (MainFunc)GetProcAddress(hModuleDll, "Main");
            if( !Main )
            {
                break;
            }
            result = (int)Main(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

        } while (false);

        if (result == INT_MIN)
        {
            LPSTR messageBuffer = nullptr;
            DWORD size = FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&messageBuffer,
                0,
                NULL);
            MessageBoxA(NULL, messageBuffer, "Call module error", MB_OK | MB_ICONERROR);
            LocalFree(messageBuffer);
        }

        if (hModuleDll)
            FreeLibrary(hModuleDll);

        return result;
    }

    int ShowSettings()
    {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        return CallLibrary(Constants::AnyFseSettingsDll, GetModuleHandle(NULL), NULL, NULL, 0);;
    }

    void InitCustomControls()
    {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_STANDARD_CLASSES;
        ::InitCommonControlsEx(&icex);
    }

    bool IsFirstLaunch()
    {
        static int isFirstLaunch = -1;

        if (isFirstLaunch == -1)
        {
            if (!GlobalFindAtom(Constants::PackageAtomName))
            {
                GlobalAddAtom(Constants::PackageAtomName);
                log.Debug("First launch registered at %s experience mode", GamingExperience::IsFullscreenMode() ? "Fullscreen" : "Desktop");
                isFirstLaunch = 1;
            }
            else
            {
                log.Debug("Subsequence launch at %s experience mode", GamingExperience::IsFullscreenMode() ? "Fullscreen" : "Desktop");
                isFirstLaunch = 0;
            }
        }
        return isFirstLaunch == 1;
    }

    bool IsRestarted()
    {
        bool restartDetected = false;
        if (Launchers::IsPlaynite(Config::Launcher.Type) && GamingExperience::IsFullscreenMode() && !IsFirstLaunch())
        {
            log.Debug("Looking for Playnite process");

            restartDetected = Launchers::HasLauncherProcess();
            if (!restartDetected)
            {
                Sleep(500);
                log.Debug("Once more Looking for Playnite process");
                restartDetected = Launchers::HasLauncherProcess();
            }
        }
        if (restartDetected)
        {
            log.Debug("Restarting Playnite is detected");
        }

        return restartDetected;
    }

    bool RunStartupApps()
    {
        log.Trace("Run startup apps");

        if (Launchers::HasStartupApps(true))
        {
            log.Trace("Trigger elevated startup apps");
            if (!Elevated::ElevatedStartupApps())
            {
                log.Error("Failed to launch elevated startup applications");
            }
        }
        Launchers::LaunchStartupApps(false);
        return false;
    }

    bool ApiIsAvailable(HINSTANCE hInstance)
    {
        if (!GamingExperience::ApiIsAvailable)
        {
            log.Critical("Fullscreen Gaming API is not detected, exiting\n");
            InitCustomControls();
            TaskDialog(NULL, hInstance,
                       L"Error",
                       L"Gaming Fullscreen Experiense API is not detected",
                       L"Fullscreen experiense is not available on your version of windows.\n"
                       L"It is supported since Windows 25H2 version for Handheld Devices",
                       TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, NULL);
            return false;
        }
        log.Debug("Compatibility checks passed");
        return true;
    }

}