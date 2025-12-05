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
#include "resource.h"
#include <tchar.h>
#include <commctrl.h>
#include <strsafe.h>

#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Process.hpp"

#include "App/Application.hpp"
#include "AppSettings/SettingsDialog.hpp"
#include "AppSettings.hpp"
#include "Tools/TaskManager.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    return TRUE;
}

__declspec(dllexport)
int WINAPI Main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    using namespace AnyFSE::App::AppSettings;
    return AppSettings::AsSettings(lpCmdLine) || AppSettings::AsRestartTask(lpCmdLine)
               ? AppSettings::WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
               : 0;
};

namespace AnyFSE::App::AppSettings
{
    static Logger log = LogManager::GetLogger("AppSettings");

    HWND AppSettings::m_hDialogWnd = nullptr;

    bool AppSettings::AsRestartTask(LPSTR lpCmdLine)
    {
        return _strcmpi(lpCmdLine, "/RestartTask") == 0;
    }

    bool AppSettings::AsSettings(LPSTR lpCmdLine)
    {
        return _strcmpi(lpCmdLine, "/Settings") == 0;
    }

    LRESULT AppSettings::AdminWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == UserMessage::WM_RESTARTTASK)
        {
            return RestartTask();
        }
        else if (uMsg == UserMessage::WM_SHOWSETTINGS)
        {
            return ShowSettings();
        }
        else if (uMsg == WM_DESTROY)
        {
            if (m_hDialogWnd)
            {
                DestroyWindow(m_hDialogWnd);
            }
            log.Debug("Destroy Admin layer");
            PostQuitMessage(0);
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    void AppSettings::PostCommand(HWND hAdminWnd, LPSTR lpCmdLine)
    {
        if (AppSettings::AsSettings(lpCmdLine))
        {
            log.Debug("Post WM_SHOWSETTINGS");
            PostMessage(hAdminWnd, UserMessage::WM_SHOWSETTINGS, 0, 0);
        }
        else if (AppSettings::AsRestartTask(lpCmdLine))
        {
            log.Debug("Post WM_RESTARTTASK");
            PostMessage(hAdminWnd, UserMessage::WM_RESTARTTASK, 0, 0);
        }
    }

    int WINAPI AppSettings::WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
    {
        bool windowCreated = false;
        HWND hAdminWnd = FindWindow(AnyFSEAdminWndClassName, nullptr);

        if (hAdminWnd)
        {
            PostCommand(hAdminWnd, lpCmdLine);
            return 0;
        }

        Config::Load();
        Config::GetStartupConfigured();

        LogManager::Initialize("AnyFSE.Settings", Config::LogLevel, Config::LogPath);

        log.Debug("\n\nAppSettings layer is started (hInstance=%08x)", hInstance);
        log.Debug("Messages ID: WM_RESTARTTASK: %u, WM_SHOWSETTINGS: %u", UserMessage::WM_RESTARTTASK, UserMessage::WM_SHOWSETTINGS );

        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = AdminWndProc;
        wc.lpszClassName = AnyFSEAdminWndClassName;
        wc.hInstance = GetModuleHandle(NULL);

        RegisterClassEx(&wc);

        hAdminWnd = CreateWindowEx(0, AnyFSEAdminWndClassName,
                                  L"Admin procedure of AnyFSE",
                                  0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

        log.Debug("Admin window is created  (hWnd=%08x)", hAdminWnd);

        PostCommand(hAdminWnd, lpCmdLine);

        MSG stMsg;
        while (GetMessage(&stMsg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&stMsg);
            DispatchMessage(&stMsg);
        }

        log.Debug("Admin window is exiting");

        UnregisterClass(AnyFSEAdminWndClassName, GetModuleHandle(NULL));

        return (int)stMsg.wParam;
    }

    int AppSettings::ShowSettings()
    {
        log.Debug("Show Settings");
        if (m_hDialogWnd)
        {
            log.Debug("Dialog is active already, show it");
            ShowWindow(m_hDialogWnd, SW_SHOWNORMAL);
            SetForegroundWindow(m_hDialogWnd);
            SetActiveWindow(m_hDialogWnd);
            return 0;
        }

        log.Debug("Creating dialog");
        int result = (int)Settings::SettingsDialog(m_hDialogWnd).Show(GetModuleHandle(NULL));
        log.Debug("Dialog completed: %d", result);
        m_hDialogWnd = nullptr;
        if (result == IDOK)
        {
            RestartApp();
        }
        return result;
    }

    int AppSettings::RestartTask()
    {
        log.Debug("Restarting task");
        TaskManager::RestartTask();
        return 0;
    }

    int AppSettings::RestartApp()
    {
        log.Debug("Restarting app");
        HWND hAppWnd = FindWindow(AnyFSESplashWndClassName, NULL);
        if (hAppWnd)
        {
            PostMessage(hAppWnd, WM_DESTROY, 0, 0);
            return 0;
        }
        return RestartTask();
    }
};
