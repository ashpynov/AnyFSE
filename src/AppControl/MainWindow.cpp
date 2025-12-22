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


#include <tchar.h>
#include <windows.h>
#include <filesystem>
#include <winsock.h>
#include "resource.h"
#include "Logging/LogManager.hpp"
#include "Configuration/Config.hpp"
#include "Tools/Icon.hpp"
#include "Tools/Unicode.hpp"
#include "FluentDesign/Theme.hpp"
#include "FluentDesign/Popup.hpp"
#include "AppControl/AppControl.hpp"
#include "AppControl/MainWindow.hpp"
#include "MainWindow.hpp"
#include "Updater/Updater.hpp"
#include "Updater/Updater.AppControl.hpp"
#include "Tools/Notification.hpp"
#include "Tools/PowerEfficiency.hpp"
#include "Tools/Paths.hpp"

#pragma comment(lib, "mpr.lib")

namespace AnyFSE::App::AppControl::Window
{
    static Logger log = LogManager::GetLogger("MainWindow");
    WNDCLASS MainWindow::WC;

    MainWindow::MainWindow()
        : m_hWnd(NULL)
        , m_aClass(NULL)
        , m_pLogoImage(nullptr)
        , m_gdiplusToken(0ll)
        , WM_TASKBARCREATED(RegisterWindowMessage(L"TaskbarCreated"))
        , WM_UPDATER_COMMAND(RegisterWindowMessage(L"AnyFSE.Updater.Command"))
    {
        ZeroMemory(&WC, sizeof(WC));
    }
    MainWindow::~MainWindow()
    {
        if (WC.hIcon)
        {
            DestroyIcon(WC.hIcon);
        }

        if (IsWindow(m_hWnd))
        {

            DestroyWindow(m_hWnd);
            m_hWnd = NULL;
        }
        if (m_aClass)
        {
            UnregisterClass((LPCTSTR)m_aClass, WC.hInstance);
        }
    }

    bool MainWindow::Create(LPCWSTR className, HINSTANCE hInstance, LPCTSTR windowName)
    {
        WC.lpszClassName = className;
        WC.hInstance = hInstance;
        WC.lpfnWndProc = MainWndProc;
        WC.hIcon = Icon::LoadIcon(Config::Launcher.IconFile, 16);
        //stWC.hbrBackground = NULL_BRUSH;
        WC.style = CS_HREDRAW | CS_VREDRAW;

        m_aClass = RegisterClass(&WC);
        if (!m_aClass)
        {
            log.Debug(Logger::APIError(), "Can not register class name");
            return false;
        }

        m_hWnd = CreateWindowEx(
            WS_EX_TOPMOST,
            (LPCTSTR)m_aClass, windowName,
            WS_POPUP,
            CW_USEDEFAULT, CW_USEDEFAULT,
            0, 0,
            NULL, NULL, hInstance, this);

        if (!IsWindow(m_hWnd))
        {
            log.Debug(Logger::APIError(), "Can not create window");
            return false;
        }

        log.Debug("Window is created (hWnd=%08x)", m_hWnd);

        SelectNextVideo();
        m_videoPlayer.Load(m_currentVideo.c_str(), Config::SplashVideoMute, Config::SplashVideoLoop, Config::SplashVideoPause, m_hWnd);

        return true;
    };

    bool MainWindow::Show(bool empty)
    {
        m_empty = empty;
        if (IsWindow(m_hWnd))
        {
            m_videoPlayer.Load(m_currentVideo.c_str(), Config::SplashVideoMute, Config::SplashVideoLoop, Config::SplashVideoPause, m_hWnd);

            if (!m_empty && !m_suspended)
            {
                if (Config::SplashShowVideo)
                {
                    Tools::EnablePowerEfficencyMode(false);
                }
                m_videoPlayer.Play();
                StartAnimation();
            }
            AnimateWindow(m_hWnd, 0, AW_BLEND);
            ShowWindow(m_hWnd, SW_MAXIMIZE);
            SetActiveWindow(m_hWnd);
            SetForegroundWindow(m_hWnd);
        }
        return true;
    }

    bool MainWindow::Start()
    {
        m_empty = false;
        if (IsWindowVisible(m_hWnd))
        {
            log.Debug("Start animation");
            m_videoPlayer.Load(m_currentVideo.c_str(), Config::SplashVideoMute, Config::SplashVideoLoop, Config::SplashVideoPause, m_hWnd);
            m_videoPlayer.Play();
            StartAnimation();
        }
        return true;
    }

    bool MainWindow::Hide()
    {
        if (IsWindowVisible(m_hWnd))
        {
            AnimateWindow(m_hWnd, 200, AW_BLEND | AW_HIDE);
            m_videoPlayer.Close();
            SelectNextVideo();
            StopAnimation();
        }

        Tools::EnablePowerEfficencyMode(true);

        return true;
    }

    bool MainWindow::IsVisible()
    {
        return IsWindowVisible(m_hWnd);
    }

    int MainWindow::ExitOnError()
    {
        DWORD error = GetLastError();
        if (error)
        {
            PostMessage(m_hWnd, WM_DESTROY, (WPARAM)error, 0);
        }
        return error;
    }

    void MainWindow::StartUpdateCheck()
    {
        Updater::Subscribe(m_hWnd, WM_UPDATE_NOTIFICATION);
        ScheduleCheck();
    }

    // static
    int MainWindow::RunLoop()
    {
        MSG stMsg;
        while (GetMessage(&stMsg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&stMsg);
            DispatchMessage(&stMsg);
        }
        return (int)stMsg.wParam;
    }

    // static
    LRESULT CALLBACK MainWindow::MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        MainWindow *pWindow = nullptr;

        if (uMsg == WM_NCCREATE)
        {
            CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
            pWindow = (MainWindow *)pCreate->lpCreateParams;
            pWindow->m_hWnd = hWnd;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pWindow);

        }
        else
        {
            pWindow = (MainWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        }

        if (pWindow)
        {
            return pWindow->HandleMessage(uMsg, wParam, lParam);
        }

        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_TASKBARCREATED)
        {
            log.Debug("Explorer start taskbar ready");
            OnStartWindow.Notify();
            ExitOnError();
            return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
        }
        else if (uMsg == WM_UPDATER_COMMAND)
        {
            if (Updater::UpdaterHandleCommand(m_hWnd, wParam, lParam))
            {
                OnUpdateCheck();
            }
            return FALSE;
        };
        switch (uMsg)
        {
        case WM_CREATE:
            OnCreate();
            return 0;
        case WM_PAINT:
            OnPaint();
            break;
        case WM_ERASEBKGND:
            return 1;
        case WM_DESTROY:
            m_result = (int)wParam;
            OnDestroy();
            break;
        case WM_TRAY:
            OnTray(lParam);
            return 0;
        case WM_TIMER:
            OnTimer(wParam);
            return 0;
        case WM_SIZE:
            m_videoPlayer.Resize();
            break;
        case WM_COMMAND:
            if (OnCommand(LOWORD(wParam)))
            {
                return 0;
            }
            break;
        case WM_QUERYENDSESSION:
            log.Info("QueryEndSession recieved");
            OnQueryEndSession.Notify();
            break;
        case WM_ENDSESSION:
            log.Info("EndSession recieved");
            OnEndSession.Notify();
            break;
        case WM_UPDATE_NOTIFICATION:
            OnUpdateNotification();
            return 0;
        case WM_USER:
            if (Config::UpdateNotifications)
            {
                Notification::ShowCurrentVersion(m_hWnd, wParam);
            }
            return 0;
        case WM_UPDATE_CHECK:
            OnUpdateCheck();
            return 0;
        }
        return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
    }

    // static
    void MainWindow::LoadStringSafe(UINT nStrID, LPTSTR szBuf, UINT nBufLen)
    {
        UINT nLen = LoadString(WC.hInstance, nStrID, szBuf, nBufLen);
        if (nLen >= nBufLen)
            nLen = nBufLen - 1;
        szBuf[nLen] = 0;
    }

    void MainWindow::OnCreate()
    {
        InitAnimationResources();
    }

    void MainWindow::CreateTrayIcon()
    {
        if (m_trayCreated)
        {
            return;
        }
        SetLastError(0);
        NOTIFYICONDATA stData;
        ZeroMemory(&stData, sizeof(stData));
        stData.cbSize = sizeof(stData);
        stData.hWnd = m_hWnd;
        stData.uID = 1;
        stData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        stData.uCallbackMessage = WM_TRAY;
        stData.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
        LoadStringSafe(IDS_TIP, stData.szTip, _countof(stData.szTip));
        if (!Shell_NotifyIcon(NIM_ADD, &stData))
        {
            log.Debug("Can not create tray icon");
            return;
        }
        m_trayCreated = true;
    }

    void MainWindow::OnPaint()
    {
        if (!m_empty && (Config::SplashShowAnimation || Config::SplashShowLogo || Config::SplashShowText))
        {
            OnPaintAnimated();
        }
        else
        {
            PAINTSTRUCT ps;
            BeginPaint(m_hWnd, &ps);
            EndPaint(m_hWnd, &ps);
        }
    }

    void MainWindow::OnTray(LPARAM message)
    {
        switch (message)
        {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            SendMessage(m_hWnd, WM_COMMAND, ID_CONFIGURE, 0);
            break;
        case WM_RBUTTONDOWN:
        {
            static FluentDesign::Theme theme;
            theme.AttachWindow(m_hWnd);
            static FluentDesign::Popup popup(theme);
            static std::vector<FluentDesign::Popup::PopupItem> popupItems{
                FluentDesign::Popup::PopupItem(L"\xE713", L"Configure",
                    [This = this]() { SendMessage(This->m_hWnd, WM_COMMAND, MAKEWPARAM(ID_CONFIGURE, 0), 0); }),
                FluentDesign::Popup::PopupItem(L"\xE733", L"Quit",
                    [This = this]() { SendMessage(This->m_hWnd, WM_COMMAND, MAKEWPARAM(ID_QUIT, 0), 0); })
            };
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(m_hWnd);
            popup.Show(m_hWnd, pt.x, pt.y, popupItems, 300, TPM_LEFTALIGN | TPM_BOTTOMALIGN);
            theme.SwapFocus(popup.GetHwnd());
            break;
        }
        default:
            break;
        }
    }

    BOOL MainWindow::FreeResources()
    {
        NOTIFYICONDATA stData;
        ZeroMemory(&stData, sizeof(stData));
        stData.cbSize = sizeof(stData);
        stData.hWnd = m_hWnd;
        Shell_NotifyIcon(NIM_DELETE, &stData);
        FreeAnimationResources();
        return TRUE;
    }

    void MainWindow::OnUpdateNotification()
    {
        Updater::Subscribe(m_hWnd, WM_UPDATE_NOTIFICATION);

        Updater::UpdateInfo upd = Updater::GetLastUpdateInfo();
        if (upd.uiState == Updater::NetworkFailed &&
            ((Config::UpdateCheckInterval == 0 && !m_successChecked) || Config::UpdateCheckInterval > 0)
        )
        {
            ScheduleCheck();
        }
        else if (upd.uiState == Updater::UpdaterState::Done
                && upd.uiCommand == Updater::UpdaterState::CheckingUpdate
                )
        {
            m_successChecked = true;
            if (Config::UpdateNotifications && !upd.newVersion.empty() && upd.newVersion != Config::UpdateLastVersion)
            {
                Notification::ShowNewVersion(m_hWnd, upd.newVersion);
            }
            Config::SaveUpdateVersion(upd.newVersion);
            OnUpdateCheck();
        }
    }

    void MainWindow::OnUpdateCheck()
    {
        int delay = Updater::ScheduledCheckAsync(Config::UpdateLastCheck, Config::UpdateCheckInterval, Config::UpdatePreRelease, m_hWnd, WM_UPDATE_NOTIFICATION);
        ScheduleCheck(delay);
    }

    void MainWindow::ScheduleCheck(int delay)
    {
        KillTimer(m_hWnd, m_updateTimerId);

        if (delay <= 0)
        {
            log.Debug("Stop Updater Schedule");
            return;
        }

        log.Debug("Schedule updater check in %ds/%.2fh", delay, (float)delay / 3600);

        SetTimer(m_hWnd, m_updateTimerId, (UINT)delay * 1000,
            [](HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
            {
                KillTimer(hWnd, idEvent);
                SendMessage(hWnd, WM_UPDATE_CHECK, 0, 0);
            });
    }

    BOOL MainWindow::OnCommand(WORD command)
    {
        switch (command)
        {
        case ID_CONFIGURE:
            {
                // Ensure normal priority while settings dialog runs
                Tools::EnablePowerEfficencyMode(false);
                int result = AppControl::ShowSettings();
                // Restore idle/low-power after settings dialog closes
                Tools::EnablePowerEfficencyMode(true);

                if ( result == IDOK)
                {
                    OnReconfigure.Notify();
                    LoadLogoImage();
                    return TRUE;
                }
            }
            return FALSE;
        case ID_QUIT:
            m_result = 0;
            DestroyWindow(m_hWnd);
            return TRUE;
        }
        return FALSE;
    }

    void MainWindow::OnDestroy()
    {
        FreeResources();
        PostQuitMessage(m_result);
    }

    void MainWindow::SelectNextVideo()
    {

        if (!Config::SplashShowVideo || Config::Launcher.Type == LauncherType::ArmouryCrate)
        {
            return;
        }
        m_currentVideo = L"";

        namespace fs = std::filesystem;
        std::wstring mediaPath = Config::SplashVideoPath.empty() ? Tools::Paths::GetDataPath() + L"\\splash" : Config::SplashVideoPath;

        std::vector<fs::path> videoFiles;

        try
        {
            if (fs::exists(mediaPath) && fs::is_regular_file(mediaPath))
            {
                videoFiles.push_back(fs::path(mediaPath));
            }
            else
            {
            // Check if the directory exists
            if (!fs::exists(mediaPath) || !fs::is_directory(mediaPath))
            {
                log.Error("Error: Directory does not exist or is not a directory: %s", Unicode::to_string(mediaPath).c_str());
                return;
            }

            // Iterate through all files in the directory
            for (const auto &entry : fs::directory_iterator(mediaPath))
            {
                if (fs::is_regular_file(entry))
                {
                    std::string extension = entry.path().extension().string();

                    // Convert to lowercase for case-insensitive comparison
                    std::transform(extension.begin(), extension.end(), extension.begin(),
                                   [](unsigned char c)
                                   { return std::tolower(c); });

                    // Check if the file has .mp4 or .webm extension
                    if (extension == ".mp4" || extension == ".webm")
                    {
                        videoFiles.push_back(entry.path());
                    }
                }
            }
            }
        }
        catch (const fs::filesystem_error &ex)
        {
            log.Error(ex, "Filesystem error:");
        }
        catch (const std::exception &ex)
        {
            log.Error(ex, "Error:");
        }

        if (!videoFiles.size())
        {
            log.Warn("No video files found");
            return;
        }

        m_currentVideo = videoFiles[GetTickCount64() % videoFiles.size()].wstring().c_str();
    }

    void MainWindow::Suspend(bool bSuspend)
    {
        if (m_suspended == bSuspend)
        {
            log.Debug(">>> Resuspend");
            return;
        }

        m_suspended = bSuspend;

        if (IsWindowVisible(m_hWnd))
        {
            if (bSuspend)
            {
                Tools::EnablePowerEfficencyMode(true);
                m_videoPlayer.Pause();
                StopAnimation();
            }
            else
            {
                Tools::EnablePowerEfficencyMode(false);
                Start();
            }
        }
        else
        {
            log.Debug("Splash is not visible");
        }
    }
}
