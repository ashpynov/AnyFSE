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
#include "App/App.hpp"
#include "App/MainWindow.hpp"
#include "App/Launchers.hpp"
#include "App/Constants.hpp"
#include "MainWindow.hpp"
#include "Tools/Notification.hpp"
#include "Tools/Paths.hpp"
#include "Tools/Process.hpp"
#include "Tools/ShellWindow.hpp"
#include "Tools/DoubleBufferedPaint.hpp"
#include "GamingExperience.hpp"

#pragma comment(lib, "mpr.lib")

namespace AnyFSE::App::Window
{
    static Logger log = LogManager::GetLogger("MainWindow");
    WNDCLASS MainWindow::WC;


    bool IsRegistered()
    {
        if (FindWindow(Constants::MainWindowClass, NULL))
        {
            log.Debug("Application control is executed already, exiting\n");
            return true;
        }
        return false;
    }

    MainWindow::MainWindow(LPCWSTR className, HINSTANCE hInstance)
        : m_hWnd(NULL)
        , m_aClass(NULL)
        , m_videoPlayer(m_theme.GetColorRef(FluentDesign::Theme::Dialog))
        , LOGO_SIZE(Config::SplashLargeLogo ? LARGE_LOGO_SIZE : NORMAL_LOGO_SIZE)
        , m_pLogoImage(nullptr)
        , m_gdiplusToken(0ll)
        , WM_TASKBARCREATED(RegisterWindowMessage(L"TaskbarCreated"))
    {
        m_videoPlayer.OnPlaybackEnded = delegate(UpdateSplashActivation);
        ZeroMemory(&WC, sizeof(WC));
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        WC.lpszClassName = className;
        WC.hInstance = hInstance;
        WC.lpfnWndProc = MainWndProc;
        WC.hIcon = nullptr;
        WC.hbrBackground = nullptr;
        WC.style = CS_HREDRAW | CS_VREDRAW;
        WC.hCursor = LoadCursor(NULL, IDC_ARROW);

        m_aClass = RegisterClass(&WC);
        if (!m_aClass)
        {
            log.Debug(Logger::APIError(), "Can not register class name");
            return;
        }
    }
    MainWindow::~MainWindow()
    {
        m_videoPlayer.OnPlaybackEnded.Clear();
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

    bool MainWindow::UpdateIcon()
    {
        if (!IsWindow(m_hWnd))
            return false;

        if (WC.hIcon)
        {
            DestroyIcon(WC.hIcon);
            WC.hIcon = nullptr;
        }

        WC.hIcon = Icon::LoadIcon(Config::Launcher.IconFile, 16);

        SetClassLongPtrW(m_hWnd, GCLP_HICON, (LONG_PTR)WC.hIcon);
        SetClassLongPtrW(m_hWnd, GCLP_HICON, (LONG_PTR)WC.hIcon);

        return true;
    }

    void MainWindow::Reset()
    {
        m_result = ERROR_RESTART_APPLICATION;
        m_empty = false;
        m_successChecked = false;
        m_launcherDetected = false;
        m_closing = false;
        m_bLauncherWasActive = false;

        if (IsWindow(m_hWnd))
        {
            DestroyWindow(m_hWnd);
            m_hWnd = NULL;
        }
    }
    bool MainWindow::Create(LPCTSTR windowName)
    {
        Reset();

        m_hWnd = CreateWindowEx(
            WS_EX_TOPMOST,
            (LPCTSTR)m_aClass, windowName,
            WS_POPUP | WS_CLIPCHILDREN,
            CW_USEDEFAULT, CW_USEDEFAULT,
            0, 0,
            NULL, NULL, WC.hInstance, this);

        if (!IsWindow(m_hWnd))
        {
            log.Debug(Logger::APIError(), "Can not create window");
            return false;
        }

        UpdateIcon();

        if (!SetTimer(m_hWnd, m_launcherCheckTimerId, CHECK_INTERVAL_MS, NULL))
        {
            log.Error(Logger::APIError(), "Cannot create launcher check timer");
            return false;
        }

        log.Debug("Window is created (hWnd=%08x)", m_hWnd);

        if (Config::SplashShowVideo)
        {
            SelectNextVideo();
            m_videoPlayer.Load(m_currentVideo.c_str(), Config::SplashVideoMute, Config::SplashVideoLoop, Config::SplashVideoPause, m_hWnd);
        }

        return true;
    };

    int MainWindow::Run(const std::wstring& name, bool startLauncher)
    {
        log.Debug("Create splash window");

        if (!Create(name.c_str()))
        {
            return (int)GetLastError();
        }

        const bool startAfterShow = startLauncher && !m_currentVideo.empty();
        if (startLauncher && !startAfterShow)
            Launchers::StartLauncher(Config::AsAdmin);

        if (!Show())
            return (int)GetLastError();

        // Acquire the splash foreground lock before the launcher can activate its window.
        if (startAfterShow)
            Launchers::StartLauncher(Config::AsAdmin);

        int exitCode = MainWindow::RunLoop();
        log.Debug("Splash window loop finished.");
        return exitCode;
    }

    bool MainWindow::Show(bool empty)
    {
        m_empty = empty;
        if (IsWindow(m_hWnd))
        {

            SignalShellReady();

            if (Config::SplashShowVideo)
            {
                m_videoPlayer.Load(m_currentVideo.c_str(), Config::SplashVideoMute, Config::SplashVideoLoop, Config::SplashVideoPause, m_hWnd);
            }

            if (!m_empty)
            {
                if (Config::SplashShowVideo)
                {
                    m_videoPlayer.Play();
                }
                StartAnimation();
            }
            AnimateWindow(m_hWnd, 0, AW_BLEND);
            ShowWindow(m_hWnd, SW_MAXIMIZE);
            if (!SetTimer(m_hWnd, m_launcherTimeoutTimerId, LAUNCHER_TIMEOUT_MS, NULL))
            {
                log.Error(Logger::APIError(), "Cannot create launcher timeout timer");
                m_closing = true;
                DestroyWindow(m_hWnd);
                return false;
            }
            SetActiveWindow(m_hWnd);
            SetForegroundWindow(m_hWnd);
            UpdateSplashActivation();
        }
        return true;
    }

    void MainWindow::SignalShellReady()
    {
        UpdateWindow(m_hWnd);

        HANDLE event = OpenEvent(EVENT_MODIFY_STATE, FALSE, Constants::ShellDesktopSwitchEvent);
        if (!event)
        {
            log.Warn(Logger::APIError(), "Cannot open shell readiness event %ls", Constants::ShellDesktopSwitchEvent);
            return;
        }

        SetEvent(event);

        log.Info("Signaled shell readiness event %ls", Constants::ShellDesktopSwitchEvent);

        CloseHandle(event);
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
        };
        switch (uMsg)
        {
        case WM_CREATE:
            OnCreate();
            return 0;
        case WM_PAINT:
            OnPaint();
            return 0;
        case WM_ANIMATION_FRAME:
            if (m_animating)
            {
                RECT box = LogoRect();
                InvalidateRect(m_hWnd, &box, FALSE);
            }
            return 0;
        case WM_PRINTCLIENT:
            OnPrintClient(reinterpret_cast<HDC>(wParam));
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_DESTROY:
            m_result = (int)wParam;
            OnDestroy();
            break;
        case WM_TIMER:
            OnTimer(wParam);
            return 0;
        case WM_SIZE:
            m_videoPlayer.Resize();
            break;
        case WM_DPICHANGED:
        {
            m_theme.UpdateDpi(HIWORD(wParam));
            const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
            SetWindowPos(m_hWnd, NULL, suggested->left, suggested->top,
                suggested->right - suggested->left, suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
            InvalidateRect(m_hWnd, NULL, FALSE);
            return 0;
        }
        case WM_WINDOWPOSCHANGING:
            if (ShouldKeepSplashActive())
            {
                // Keep the video above other windows even if the shell changes our Z order during activation.
                WINDOWPOS* position = reinterpret_cast<WINDOWPOS*>(lParam);
                position->hwndInsertAfter = HWND_TOPMOST;
                position->flags &= ~SWP_NOZORDER;
            }
            break;
        case WM_ACTIVATE:
            if (LOWORD(wParam) != WA_INACTIVE && ShouldKeepSplashActive())
            {
                if (LockSetForegroundWindow(LSFW_LOCK))
                    m_foregroundLocked = true;
            }
            if (LOWORD(wParam) == WA_INACTIVE && ShouldKeepSplashActive())
            {
                // Preserve coverage immediately; queued focus recovery must not expose the launcher for a frame.
                SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                PostMessage(m_hWnd, WM_KEEP_SPLASH_ACTIVE, 0, 0);
                return 0;
            }
            break;
        case WM_KEEP_SPLASH_ACTIVE:
            UpdateSplashActivation();
            return 0;
        }
        return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
    }

    bool MainWindow::ShouldKeepSplashActive()
    {
        return !m_closing && !m_empty && GamingExperience::IsFullscreenMode()
            && Config::SplashShowVideo && Config::SplashTillEnd && m_videoPlayer.ShouldWaitForEnd();
    }

    void MainWindow::UpdateSplashActivation()
    {
        if (ShouldKeepSplashActive())
        {
            // Keep the shell view active too: uncloaking alone can stall Steam's presentation after the video.
            const HRESULT result = ShellWindow::SwitchTo(m_hWnd);
            if (FAILED(result))
                log.Warn("Cannot activate splash shell view: HRESULT=%#lx", static_cast<unsigned long>(result));

            SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            // Do not use the generic foreground helper: its simulated Alt key releases the foreground lock.
            if (GetForegroundWindow() != m_hWnd)
                SetForegroundWindow(m_hWnd);
            if (GetForegroundWindow() == m_hWnd && LockSetForegroundWindow(LSFW_LOCK))
                m_foregroundLocked = true;
        }
        else
        {
            if (m_foregroundLocked)
            {
                LockSetForegroundWindow(LSFW_UNLOCK);
                m_foregroundLocked = false;
            }

            if (!m_empty && Config::SplashShowVideo && Config::SplashTillEnd && !m_currentVideo.empty() && GamingExperience::IsFullscreenMode())
            {
                // Hand the active shell view back after the video so the launcher resumes presentation.
                SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                HWND launcher = Launchers::GetLauncherWindow(true);
                if (launcher)
                {
                    const HRESULT result = ShellWindow::SwitchTo(launcher);
                    if (FAILED(result))
                        log.Warn("Cannot switch to launcher shell view: HRESULT=%#lx", static_cast<unsigned long>(result));
                }
            }
        }
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
        m_theme.AttachWindow(m_hWnd);
        InitAnimationResources();
    }

    void MainWindow::OnDestroy()
    {
        m_closing = true;
        UpdateSplashActivation();
        m_videoPlayer.Close();
        FreeAnimationResources();
        PostQuitMessage(m_result);
    }

    void MainWindow::SelectNextVideo()
    {

        if (!Config::SplashShowVideo)
        {
            return;
        }
        m_currentVideo = L"";

        namespace fs = std::filesystem;
        std::wstring mediaPath = Config::SplashVideoPath.empty() ? Tools::Paths::GetSplashDefaultPath() : Config::SplashVideoPath;

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
                log.Error("Error: Directory does not exist or is not a directory: %ls", mediaPath.c_str());
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
}
